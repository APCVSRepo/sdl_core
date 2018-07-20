/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */
 
#ifndef EVENT_DISPATCH_H
#define EVENT_DISPATCH_H


#include <events.h>
#include <list.h>
#include <pthread.h>


class event_dispatch {
	friend class TimerNotifier;
	

private:
	ListItem	m_timers;
	ListItem	m_sockets;
	struct timeval	m_last_run;

	bool		m_sleeping;

	static void PairTimers(ListItem &siblings, ListItem &dest,
			       unsigned int delta);
	void AddTimer(IndepTimerNotifier *);
	void RemoveTimer(IndepTimerNotifier *);
	void RunTimers(unsigned int ms_elapsed);

	void AddSocket(IndepSocketNotifier *);
	void RemoveSocket(IndepSocketNotifier *);

public:
	

	void RunOnce(int max_sleep_ms = -1);
	void Run(void);

	event_dispatch(void);
	virtual ~event_dispatch();
};



#endif 