/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>



#if defined(USE_ALSA_SOUNDIO)
#include <alsa/asoundlib.h>
#endif

#if defined(USE_PTHREADS)
#define ALSA_THREADS
#include <pthread.h>
#endif




class bluetooth_audio_alsa_thread  {
	pthread_t	m_play_thread;
	pthread_t	m_rec_thread;
	TimerNotifier	*m_async_not;
	bool		m_play_idle;
	TimerNotifier	*m_play_wake;

	static void *PlayThreadHelper(void *arg) {
		bluetooth_audio_alsa_thread *objp = (bluetooth_audio_alsa_thread *) arg;
		objp->PlayThread();
		return 0;
	}
	static void *RecThreadHelper(void *arg) {
		bluetooth_audio_alsa_thread *objp = (bluetooth_audio_alsa_thread *) arg;
		objp->RecThread();
		return 0;
	}

public:
	bluetooth_audio_alsa_thread(DispatchInterface *eip,
			      const char *output_devspec,
			      const char *input_devspec){}
	virtual ~bluetooth_audio_alsa_thread() {}

	void RecThread(void) {
		unsigned int nsamples;
		uint8_t *buf;
		snd_pcm_sframes_t err;
		int res;
		ErrorInfo error;

		m_lock.Lock();
		if (m_rec_nonblock) {
			res = snd_pcm_nonblock(m_alsa.m_rec_handle, false);
			if (res < 0) {
				m_lock.Unlock();
				
				BufAbort(m_alsa.m_ei, error);
				return;
			}
			m_rec_nonblock = false;
		}

		while (m_async_not) {
			nsamples = m_alsa.m_format.packet_samps;
			m_input.GetUnfilled(buf, nsamples);
			assert(nsamples);

			m_lock.Unlock();
			err = snd_pcm_readi(m_alsa.m_rec_handle, buf,nsamples);
			m_lock.Lock();

			if (err < 0) {
				assert(err != -EAGAIN);
				if (m_alsa.HandleInterruption(
					    m_alsa.m_rec_handle, err,
					    &error)) {
					continue;
				}
				m_lock.Unlock();
				BufAbort(m_alsa.m_ei, error);
				return;
			}

			if (!err)
				continue;

			m_input.PutUnfilled(err);

			
			if (m_async_not)
				m_async_not->Set(0);
		}

		m_lock.Unlock();
	}

	void PlayThread(void) {
		unsigned int nsamples;
		uint8_t *buf;
		int res;
		snd_pcm_sframes_t err;
		bool underrun;
		ErrorInfo error;
		unsigned int msec;

		m_lock.Lock();
		if (m_play_nonblock) {
			res = snd_pcm_nonblock(m_alsa.m_play_handle, false);
			if (res < 0) {
				m_lock.Unlock();
				
				BufAbort(m_alsa.m_ei, error);
				return;
			}
			m_play_nonblock = false;
		}

		msec = (m_alsa.m_format.packet_samps * 1000) /
			m_alsa.m_format.samplerate;

		while (m_async_not) {
			m_async_not->Set(0);
			(void) snd_pcm_avail_update(m_alsa.m_play_handle);
			if (!m_alsa.CheckXrun(m_alsa.m_play_handle,
					      underrun,
					      &error)) {
				m_lock.Unlock();
				BufAbort(m_alsa.m_ei, error);
				return;
			}
			m_alsa.m_play_xrun = underrun;
			m_hw_outq = m_alsa.GetPlaybackQueue();
			nsamples = m_alsa.m_format.packet_samps;
			m_output.Peek(buf, nsamples);
			if (!nsamples) {
				
				m_play_idle = true;
				m_play_wake->Set(msec / 2);
				m_lock.Wait();
				continue;
			}

			if (m_play_idle) {
				m_play_wake->Cancel();
				m_play_idle = false;
			}

			m_lock.Unlock();
			err = snd_pcm_writei(m_alsa.m_play_handle,
					     buf, nsamples);
			m_lock.Lock();

			if (err < 0) {
				assert(err != -EAGAIN);
				if (m_alsa.HandleInterruption(
					    m_alsa.m_play_handle, err,
					    &error))
					continue;

				m_lock.Unlock();
				BufAbort(m_alsa.m_ei, error);
				return;
			}
			if (!err) {
				m_lock.Unlock();
				
				BufAbort(m_alsa.m_ei, error);
				return;
			}

			m_output.Dequeue(err);
		}

		m_lock.Unlock();
	}

	void PlayThreadWakeup(TimerNotifier *notp) {
		m_lock.Lock();
		if (m_play_idle) {
			m_play_idle = false;
			m_lock.Signal();
		}
		m_lock.Unlock();
	}

	virtual void SndPushInput(bool nonblock) {
		
	}

	virtual void SndPushOutput(bool nonblock) {
		
		m_lock.Lock();
		if (m_play_idle && m_output.TotalFill())
			m_lock.Signal();
		m_lock.Unlock();
	}

	void AsyncProcess(TimerNotifier *notp) {
		bool overrun = false, underrun = false;
		OpLatencyMonitor olat(m_alsa.m_ei, "ALSA async overall");

		m_lock.Lock();

		if (m_abort)
			goto do_abort;

		if (m_rec_thread) {
			overrun = m_alsa.m_rec_xrun;
			m_alsa.m_rec_xrun = false;
		}

		if (m_play_thread) {
			underrun = m_alsa.m_play_xrun;
			m_alsa.m_play_xrun = false;
		}

		if (!BufProcess(m_hw_outq, overrun, underrun))
			return;

		m_lock.Unlock();
		return;

	do_abort:
		SndHandleAbort(m_abort);
		m_lock.Unlock();
	}

	virtual bool SndAsyncStart(bool playback, bool capture,
				   ErrorInfo *error) {
		int res;

		m_play_idle = false;

		m_async_not = m_alsa.m_ei->NewTimer();
		if (!m_async_not) {
			if (error)
				error->SetNoMem();
			return false;
		}

		m_async_not->Register(this,
				      &bluetooth_audio_alsa_thread::AsyncProcess);

		if (capture) {
			res = pthread_create(&m_rec_thread, 0,
				     &bluetooth_audio_alsa_thread::RecThreadHelper,
					     this);
			if (res) {
				
				SndAsyncStop();
				return false;
			}
		}
		if (playback) {
			m_play_wake = m_alsa.m_ei->NewTimer();
			if (!m_play_wake) {
				if (error)
					error->SetNoMem();
				SndAsyncStop();
				return false;
			}

			m_play_wake->Register(this,
				&bluetooth_audio_alsa_thread::PlayThreadWakeup);

			m_play_idle = true;
			res = pthread_create(&m_play_thread, 0,
				     &bluetooth_audio_alsa_thread::PlayThreadHelper,
					     this);
			if (res) {
				
				SndAsyncStop();
				return false;
			}
		}
		return true;
	}

	virtual void SndAsyncStop(void) {
		int res;

		m_lock.Lock();
		if (!m_async_not) {
			m_lock.Unlock();
			return;
		}

		delete m_async_not;
		m_async_not = 0;
		m_lock.Signal();
		m_lock.Unlock();

		if (m_rec_thread) {
			res = pthread_join(m_rec_thread, 0);
			assert(!res);
			m_rec_thread = 0;
		}
		if (m_play_thread) {
			res = pthread_join(m_play_thread, 0);
			assert(!res);
			m_play_thread = 0;
		}
		BufClose();

		if (m_play_wake) {
			delete m_play_wake;
			m_play_wake = 0;
		}
	}

	virtual bool SndIsAsyncStarted(void) const {
		return m_async_not != 0;
	}
};
#endif /* defined(ALSA_THREADS) */

