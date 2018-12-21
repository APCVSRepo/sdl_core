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



class bluetooth_audio_alsa_process  {
protected:
	bluetooth_audio_alsa		m_alsa;

	bool			m_play_nonblock;
	bool			m_rec_nonblock;

	void OpenBuf(void) {
		this->BufOpen(m_alsa.GetPacketSize(),
			      m_alsa.m_format.bytes_per_record);
	}

public:
	bluetooth_audio_alsa_process(DispatchInterface *eip,
			    const char *output_devspec,
			    const char *input_devspec)
		: m_alsa(eip, output_devspec, input_devspec) {}
	virtual ~bluetooth_audio_alsa_process() {}

	virtual bool SndOpen(bool play, bool capture, ErrorInfo *error) {
		if (play)
			m_alsa.m_play_props.bufsize = 0;
		if (capture)
			m_alsa.m_rec_props.bufsize = 0;
		if (m_alsa.OpenDevice(SND_PCM_ACCESS_RW_INTERLEAVED,
				      play, capture, error)) {
			m_play_nonblock = false;
			m_rec_nonblock = false;
			OpenBuf();
			return true;
		}
		return false;
	}
	virtual void SndClose(void) {
		this->SndAsyncStop();
		this->BufClose();
		m_alsa.CloseDevice();
	}

	virtual void SndGetFormat(SoundIoFormat &format) const {
		format = m_alsa.m_format;
		format.packet_samps = m_alsa.GetPacketSize();
	}

	virtual bool SndSetFormat(SoundIoFormat &format, ErrorInfo *error) {
		this->SndAsyncStop();
		if (m_alsa.m_play_handle)
			m_alsa.m_play_props.bufsize = 0;
		if (m_alsa.m_rec_handle)
			m_alsa.m_rec_props.bufsize = 0;
		if (m_alsa.Reconfigure(&format,
				       SND_PCM_ACCESS_RW_INTERLEAVED, error)) {
			OpenBuf();
			return true;
		}
		return false;
	}

	virtual void SndGetProps(SoundIoProps &props) const {
		m_alsa.GetProps(props);
	}
};

class SoundIoAlsaProcAsync : public bluetooth_audio_alsa_process<SoundIoBufferBase> {
public:
	SoundIoAlsaProcAsync(DispatchInterface *eip,
			     const char *output_devspec,
			     const char *input_devspec)
		: bluetooth_audio_alsa_process<SoundIoBufferBase>(eip, output_devspec, input_devspec) {}
	virtual ~SoundIoAlsaProcAsync() {}

	virtual void SndGetQueueState(SoundIoQueueState &qs) {
		if (m_alsa.m_play_handle) {
			OpLatencyMonitor lat(m_alsa.m_ei,
					     "ALSA get playback queue state");
			(void) snd_pcm_avail_update(m_alsa.m_play_handle);
			this->m_hw_outq = m_alsa.GetPlaybackQueue();
		}
		SoundIoBufferBase::SndGetQueueState(qs);
	}

	virtual void SndPushInput(bool nonblock) {
		unsigned int nsamples;
		uint8_t *buf;
		snd_pcm_sframes_t exp, err;
		ErrorInfo error;
		OpLatencyMonitor lat(m_alsa.m_ei, "ALSA SndPushInput");

		if (m_abort)
			return;		

		if (m_rec_nonblock != nonblock) {
			err = snd_pcm_nonblock(m_alsa.m_rec_handle, nonblock);
			if (err < 0) {
				
				BufAbort(m_alsa.m_ei, error);
				return;
			}
			m_rec_nonblock = nonblock;
		}

		
	restart_me:
		exp = snd_pcm_avail_update(m_alsa.m_rec_handle);
		if (exp < 0) {
			err = exp;
			goto do_interruption;
		}
		while (1) {
			nsamples = 0;
			m_input.GetUnfilled(buf, nsamples);
			assert(nsamples);
			if (exp < (snd_pcm_sframes_t) nsamples)
				break;

			err = snd_pcm_readi(m_alsa.m_rec_handle, buf,nsamples);
			if (err < 0) {
				if (err == -EAGAIN) { break; }
			do_interruption:
				if (m_alsa.HandleInterruption(
					    m_alsa.m_rec_handle, err,
					    &error)) {
					goto restart_me;
				}
				
				BufAbort(m_alsa.m_ei, error);
				break;
			}
			if (!err) { break; }

			m_input.PutUnfilled(err);
			exp -= err;
		}
	}

	virtual void SndPushOutput(bool nonblock) {
		unsigned int nsamples;
		uint8_t *buf;
		ssize_t err;
		ErrorInfo error;
		OpLatencyMonitor lat(m_alsa.m_ei, "ALSA SndPushOutput");

		if (m_abort)
			return;		

		if (!m_alsa.m_play_handle)
			return;

		if (m_play_nonblock != nonblock) {
			err = snd_pcm_nonblock(m_alsa.m_play_handle, nonblock);
			if (err < 0) {
				
				BufAbort(m_alsa.m_ei, error);
				return;
			}
			m_play_nonblock = nonblock;
		}

		(void) snd_pcm_avail_update(m_alsa.m_play_handle);
		while (1) {
			nsamples = 0;
			m_output.Peek(buf, nsamples);
			if (!nsamples) { break; }

		restart_me:
			err = snd_pcm_writei(m_alsa.m_play_handle,
					     buf, nsamples);
			if (err < 0) {
				if (err == -EAGAIN) {
					
					break;
				}
				if (m_alsa.HandleInterruption(
					    m_alsa.m_play_handle, err,
					    &error)) {
					goto restart_me;
				}
				
				BufAbort(m_alsa.m_ei, error);
				break;
			}
			if (!err) {
				
				break;
			}

			m_output.Dequeue(err);
			m_hw_outq += err;
		}
	}

	void AsyncProcess(SocketNotifier *, int ) {
		bool overrun = false, underrun = false;
		snd_pcm_sframes_t exp = 0;
		OpLatencyMonitor olat(m_alsa.m_ei, "ALSA async overall");

		

		if (m_abort)
			goto do_abort;

		if (!m_alsa.CheckNotifications())
			return;

		if (m_alsa.m_rec_async) {
			if (!m_alsa.m_rec_xrun) {
				
				SndPushInput(true);
				if (m_abort)
					goto do_abort;
			}

			if (!m_alsa.CheckXrun(m_alsa.m_rec_handle,
					      overrun,
					      &m_abort))
				goto do_abort;
		}

		if (m_alsa.m_play_async) {
			OpLatencyMonitor lat(m_alsa.m_ei,
					     "ALSA check playback");
			(void) snd_pcm_avail_update(m_alsa.m_play_handle);
			if (!m_alsa.CheckXrun(m_alsa.m_play_handle,
					      underrun,
					      &m_abort))
				goto do_abort;
			exp = m_alsa.GetPlaybackQueue();
		}

		if (!BufProcess(exp, overrun, underrun))
			return;

		if (m_alsa.m_play_not &&
		    (m_hw_outq < m_alsa.m_play_props.bufsize)) {
			
			exp = ((m_alsa.m_play_props.bufsize - m_hw_outq) +
			       m_alsa.m_play_props.packetsize);

			m_alsa.SetAvailMin(m_alsa.m_play_handle, exp);
		}
		return;

	do_abort:
		SndHandleAbort(m_abort);
	}

	virtual bool SndAsyncStart(bool playback, bool capture,
				   ErrorInfo *error) {
		Callback<void, SocketNotifier*, int> tmpl;
		if (playback) {
			m_alsa.SetAvailMin(m_alsa.m_play_handle,
					   m_alsa.m_play_props.bufsize -
					   m_alsa.m_play_props.packetsize);
		}
		tmpl.Register(this, &SoundIoAlsaProcAsync::AsyncProcess);
		if (!m_alsa.Prepare(playback, capture, error))
			return false;
		return m_alsa.CreatePcmNotifiers(playback, capture, tmpl,
						 error);
	}

	virtual void SndAsyncStop(void) {
		BufClose();
		m_alsa.CleanupPcmNotifiers();
	}

	virtual bool SndIsAsyncStarted(void) const {
		return m_alsa.HasNotifiers();
	}
};