/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */


#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <include/event_dispatch.h>




void event_dispatch::PairTimers(ListItem &siblings, ListItem &dest, unsigned int delta)
{
	ListItem newsublist;
	TimerNotifier *a;

	assert(!siblings.Empty());

	
	do {
		a = GetContainer(siblings.next,
				 TimerNotifier, m_links);
		a->m_links.UnlinkOnly();
		a->m_msec_delta += delta;

		if (!siblings.Empty()) {
			TimerNotifier *b;
			b = GetContainer(siblings.next, TimerNotifier,
				      m_links);
			b->m_links.UnlinkOnly();
			b->m_msec_delta += delta;

			if (b->m_msec_delta < a->m_msec_delta) {
				TimerNotifier *t = a; a = b; b = t;
			}

			a->m_children.AppendItem(b->m_links);
			b->m_msec_delta -= a->m_msec_delta;
		}

		newsublist.AppendItem(a->m_links);

	} while (!siblings.Empty());

	
	a = GetContainer(newsublist.prev, TimerNotifier, m_links);
	a->m_links.UnlinkOnly();

	while (!newsublist.Empty()) {
		TimerNotifier *b;
		b = GetContainer(newsublist.prev, TimerNotifier,
				 m_links);
		b->m_links.UnlinkOnly();

		if (b->m_msec_delta < a->m_msec_delta) {
			b->m_children.PrependItem(a->m_links);
			a->m_msec_delta -= b->m_msec_delta;
			a = b;
		} else {
			a->m_children.PrependItem(b->m_links);
			b->m_msec_delta -= a->m_msec_delta;
		}
	}

	dest.AppendItem(a->m_links);
}

void event_dispatch::AddTimer(TimerNotifier *timerp)
{
	assert(timerp->m_links.Empty());
	assert(timerp->m_children.Empty());

	if (!m_timers.Empty()) {
		TimerNotifier *root_to =
			GetContainer(m_timers.next, TimerNotifier,
				     m_links);
		if (root_to->m_msec_delta < timerp->m_msec_delta) {
			root_to->m_children.PrependItem(timerp->m_links);
			timerp->m_msec_delta -= root_to->m_msec_delta;
			return;
		} else {
			root_to->m_links.UnlinkOnly();
			timerp->m_children.AppendItem(root_to->m_links);
			root_to->m_msec_delta -= timerp->m_msec_delta;
		}
	}

	m_timers.AppendItem(timerp->m_links);
}

void event_dispatch::RemoveTimer(TimerNotifier *timerp)
{
	assert(!timerp->m_links.Empty());
	if (!timerp->m_children.Empty())
		PairTimers(timerp->m_children, timerp->m_links,
			   timerp->m_msec_delta);
	assert(timerp->m_children.Empty());
	timerp->m_links.Unlink();
}

void event_dispatch::RunTimers(unsigned int ms_elapsed)
{
	ListItem runlist;
	ListItem *listp;

	assert(m_timers.Length() <= 1);

	ListForEach(listp, &m_timers) {
		TimerNotifier *to, *to2;
		to = GetContainer(listp, TimerNotifier, m_links);

		
		if (to->m_msec_delta <= ms_elapsed) {
			listp = listp->prev;
			while (!to->m_children.Empty()) {
				to2 = GetContainer(to->m_children.next,
						   TimerNotifier,
						   m_links);
				to2->m_links.UnlinkOnly();
				to2->m_msec_delta += to->m_msec_delta;
				to2->m_links.UnlinkOnly();
				m_timers.AppendItem(to2->m_links);
			}
			to->m_links.UnlinkOnly();
			runlist.AppendItem(to->m_links);
		} else {
			to->m_msec_delta -= ms_elapsed;
		}
	}

	if (m_timers.next != m_timers.prev) {
		ListItem tlist;
		tlist.PrependItemsFrom(m_timers);
		PairTimers(tlist, m_timers, 0);
		assert(tlist.Empty());
		assert(m_timers.Length() == 1);
	}

	while (!runlist.Empty()) {
		TimerNotifier *to;
		to = GetContainer(runlist.next, TimerNotifier, m_links);
		to->m_links.Unlink();
		Unlock();
		(*to)(to);
		Lock();
	}
}



class TimerNotifier : public TimerNotifier {
public:
	ListItem		m_links;
	ListItem		m_children;
	unsigned int		m_msec_delta;
	event_dispatch	*m_dispatcher;

	virtual void Set(int msec) {
		m_dispatcher->Lock();
		if (!m_links.Empty())
			m_dispatcher->RemoveTimer(this);
		m_msec_delta = msec;
		m_dispatcher->AddTimer(this);
		m_dispatcher->Unlock(true);
	}
	virtual void Cancel(void) {
		m_dispatcher->Lock();
		if (!m_links.Empty())
			m_dispatcher->RemoveTimer(this);
		m_dispatcher->Unlock();
	}
	TimerNotifier(event_dispatch *disp)
		: m_dispatcher(disp) {}
	virtual ~TimerNotifier() {
		Cancel();
	}
};




void event_dispatch::AddSocket(IndepSocketNotifier *sockp)
{
	assert(sockp->m_links.Empty());
	m_sockets.AppendItem(sockp->m_links);
}

void event_dispatch::RemoveSocket(IndepSocketNotifier *sockp)
{
	
	assert(!sockp->m_links.Empty());
	sockp->m_links.Unlink();
}






void event_dispatch::RunOnce(int max_sleep_ms)
{
	fd_set readi, writei;
	struct timeval timeout, *top, etime;
	ListItem iorun;
	unsigned int ms_elapsed;
	int maxfh, res;

	FD_ZERO(&readi);
	FD_ZERO(&writei);

	Lock();
	assert(!m_sleeping);


	RunTimers(0);

	if (m_timers.Empty() && m_sockets.Empty() && (max_sleep_ms < 0)) {
		
		Unlock();
		return;
	}

	
	maxfh = 0;
	while (!m_sockets.Empty()) {
		IndepSocketNotifier *sp;
		sp = GetContainer(m_sockets.next, IndepSocketNotifier,
				  m_links);
		if (sp->m_writable)
			FD_SET(sp->m_fh, &writei);
		else
			FD_SET(sp->m_fh, &readi);

		if (maxfh < sp->m_fh)
			maxfh = sp->m_fh;

		sp->m_links.UnlinkOnly();
		iorun.AppendItem(sp->m_links);
	}

	if (m_timers.Empty()) {
		top = NULL;
		if (max_sleep_ms >= 0) {
			ms_elapsed = max_sleep_ms;
			top = &timeout;
		}
	} else {
		TimerNotifier *to;
		to = GetContainer(m_timers.next, TimerNotifier, m_links);

		
		gettimeofday(&etime, NULL);
		if (timercmp(&m_last_run, &etime, >)) {
			
			m_last_run = etime;
			ms_elapsed = 0;
		} else {
			timersub(&etime, &m_last_run, &timeout);
			ms_elapsed = ((timeout.tv_sec * 1000) +
				      (timeout.tv_usec / 1000));
		}

		ms_elapsed = (to->m_msec_delta > ms_elapsed)
			? to->m_msec_delta - ms_elapsed : 0;

		if ((max_sleep_ms >= 0) &&
		    (ms_elapsed > (unsigned int) max_sleep_ms))
			ms_elapsed = max_sleep_ms;

		top = &timeout;
	}

	if (top) {
		timeout.tv_sec = ms_elapsed / 1000;
		timeout.tv_usec = (ms_elapsed % 1000) * 1000;
	}

	m_sleeping = true;
	Unlock();

	res = select(maxfh + 1, &readi, &writei, NULL, top);

	if (res < 0) {
		if ((errno != EINTR) &&
		    (errno != ETIMEDOUT)) {
			LogWarn("Dispatch: select: %s\n", strerror(errno));
		}

		FD_ZERO(&readi);
		FD_ZERO(&writei);
	}

	Lock();
	m_sleeping = false;

	
	gettimeofday(&etime, NULL);
	if (timercmp(&m_last_run, &etime, >)) {
		
		ms_elapsed = 0;
	} else {
		timersub(&etime, &m_last_run, &timeout);
		ms_elapsed = ((timeout.tv_sec * 1000) +
			      (timeout.tv_usec / 1000));
	}

	if (ms_elapsed) {
		
		m_last_run = etime;
	}

	
	RunTimers(ms_elapsed);

	
	while (!iorun.Empty()) {
		IndepSocketNotifier *sp;
		sp = GetContainer(iorun.next, IndepSocketNotifier, m_links);
		sp->m_links.UnlinkOnly();
		m_sockets.AppendItem(sp->m_links);

		if ((sp->m_writable && FD_ISSET(sp->m_fh, &writei)) ||
		    (!sp->m_writable && FD_ISSET(sp->m_fh, &readi))) {
			Unlock();
			(*sp)(sp, sp->m_fh);
			Lock();
		}
	}

	Unlock();
}

void event_dispatch::Run(void)
{
	bool empty;

	while (1) {
		Lock();
		empty = (m_timers.Empty() && m_sockets.Empty());
		Unlock();

		if (empty)
			return;

		RunOnce(-1);
	}
}

event_dispatch::event_dispatch(void)
{
	if (!WakeSetup())
		abort();
	gettimeofday(&m_last_run, NULL);
}

event_dispatch::~event_dispatch()
{
	WakeCleanup();
}