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



class bluetooth_audio_alsa {
public:
	struct AlsaChannelProps {
		snd_pcm_uframes_t	packetsize;
		snd_pcm_uframes_t	bufsize;
		snd_pcm_uframes_t	min_avail;
	};

	SoundIoFormat			m_format;

	DispatchInterface		*m_ei;

	snd_pcm_t			*m_play_handle;
	char				*m_play_devspec;
	AlsaChannelProps		m_play_props;
	int				m_play_not_count;
	SocketNotifier			**m_play_not;
	bool				m_play_async;
	bool				m_play_xrun;

	snd_pcm_t			*m_rec_handle;
	char				*m_rec_devspec;
	AlsaChannelProps		m_rec_props;
	int				m_rec_not_count;
	SocketNotifier			**m_rec_not;
	bool				m_rec_async;
	bool				m_rec_xrun;

	bluetooth_audio_alsa(DispatchInterface *eip,
		   const char *output_devspec, const char *input_devspec)
		: m_ei(eip), 
		  m_play_handle(NULL), m_play_not_count(0), m_play_not(NULL),
		  m_play_async(false), m_rec_handle(NULL), m_rec_not_count(0),
		  m_rec_not(NULL), m_rec_async(false) {
		m_play_devspec = strdup(output_devspec);
		m_rec_devspec = m_play_devspec;
		if (input_devspec &&
		    strcmp(input_devspec, output_devspec)) {
			m_rec_devspec = strdup(input_devspec);
		}
		memset(&m_play_props, 0, sizeof(m_play_props));
		memset(&m_rec_props, 0, sizeof(m_rec_props));

		/* Set a default format */
		memset(&m_format, 0, sizeof(m_format));
		m_format.sampletype = SIO_PCM_S16_LE;
		m_format.samplerate = 8000;
		m_format.packet_samps = 128;
		m_format.nchannels = 1;
		m_format.bytes_per_record = 2;
	}

	~bluetooth_audio_alsa() {
		CloseDevice();
		if (m_play_devspec) {
			free(m_play_devspec);
		}
		if (m_rec_devspec && (m_rec_devspec != m_play_devspec)) {
			free(m_rec_devspec);
		}
	}

	snd_pcm_sframes_t GetPacketSize(void) const {
		if (m_rec_handle) return m_rec_props.packetsize;
		if (m_play_handle) return m_play_props.packetsize;
		return m_format.packet_samps;
	}

	snd_pcm_sframes_t GetSampleBytes(void) const
		{ return m_format.bytes_per_record; }

	bool OpenDevice(snd_pcm_access_t pcm_access, bool play, bool rec,
			ErrorInfo *error) {
		int err;
		if (m_play_handle || m_rec_handle) {
			if (error)
				
			return false;
		}

		
		snd_config_update_free_global();

		if (play) {
			err = snd_pcm_open(&m_play_handle, m_play_devspec,
					   SND_PCM_STREAM_PLAYBACK, 0);
			if (err < 0) {
				
				return false;
			}

			if (!ConfigurePcm(m_play_handle, m_format, pcm_access,
					  m_play_props, error)) {
				
				CloseDevice();
				return false;
			}
			m_play_xrun = false;
		}

		if (rec) {
			err = snd_pcm_open(&m_rec_handle, m_rec_devspec,
					   SND_PCM_STREAM_CAPTURE, 0);
			if (err < 0) {
				
				CloseDevice();
				return false;
			}

			if (!ConfigurePcm(m_rec_handle, m_format, pcm_access,
					  m_rec_props, error)) {
				m_ei->LogWarn("Error configuring record "
					      "device \"%s\"",
					      m_rec_devspec);
				CloseDevice();
				return false;
			}
			m_rec_xrun = false;
		}

		return true;
	}

	void CloseDevice(void) {
		if (m_play_handle) {
			snd_pcm_close(m_play_handle);
			m_play_handle = NULL;
		}
		if (m_rec_handle) {
			snd_pcm_close(m_rec_handle);
			m_rec_handle = NULL;
		}
	}

	void GetProps(SoundIoProps &props) const {
		props.has_clock = true;
		props.does_source = (m_rec_handle != NULL);
		props.does_sink = (m_play_handle != NULL);
		props.does_loop = false;
		props.remove_on_exhaust = false;
		props.outbuf_size = m_play_props.bufsize;
	}

	
	bool CreatePcmNotifiers(bool playback, bool capture,
				Callback<void, SocketNotifier*, int> &tmpl,
				ErrorInfo *error) {
		int err;
		bool do_playback = playback;

		if (!playback && !capture) {
			return true;
		}
		if (playback && (m_play_handle == NULL)) {
			if (error)
				
			return false;
		}
		if (capture && (m_rec_handle == NULL)) {
			if (error)
				
			return false;
		}

		if (playback && capture) {
			
		}

		m_play_xrun = false;
		m_rec_xrun = false;

		if (do_playback &&
		    (m_play_not == NULL) &&
		    !CreateNotifiers(m_play_handle, m_ei, tmpl, m_play_not,
				     m_play_not_count, error)) {
			return false;
		}

		if (capture && (m_rec_not == NULL)) {
			if (!CreateNotifiers(m_rec_handle, m_ei, tmpl,
					     m_rec_not, m_rec_not_count,
					     error)) {
				CleanupPcmNotifiers();
				return false;
			}

			err = snd_pcm_start(m_rec_handle);
			if (err == -EBADFD) {
				
				snd_pcm_drop(m_rec_handle);
				snd_pcm_prepare(m_rec_handle);
				err = snd_pcm_start(m_rec_handle);
			}

			if (err < 0) {
				
				CleanupPcmNotifiers();
				return false;
			}
		}

		if (!do_playback && (m_play_not != NULL)) {
			CleanupNotifiers(m_play_not, m_play_not_count);
		}

		m_play_async = playback;
		m_rec_async = capture;
		return true;
	}

	void CleanupPcmNotifiers(void) {
		m_play_async = m_rec_async = false;
		CleanupNotifiers(m_play_not, m_play_not_count);
		CleanupNotifiers(m_rec_not, m_rec_not_count);
	}

	bool Reconfigure(SoundIoFormat *formatp,
			 snd_pcm_access_t pcm_access, ErrorInfo *error) {
		SoundIoFormat format;

		if (formatp) { format = *formatp; } else { format = m_format; }

		if (m_play_handle) {
			
			snd_pcm_drop(m_play_handle);
			if (!ConfigurePcm(m_play_handle, format, pcm_access,
					  m_play_props, error)) {
				return false;
			}
			
		}

		if (m_rec_handle) {
			m_ei->LogDebug("ALSA rec state: %d",
				       snd_pcm_state(m_rec_handle));
			snd_pcm_drop(m_rec_handle);
			if (!ConfigurePcm(m_rec_handle, format, pcm_access,
					  m_rec_props, error)) {
				return false;
			}
			m_ei->LogDebug("ALSA rec state: %d",
				       snd_pcm_state(m_rec_handle));
		}

		if (!m_play_handle && !m_rec_handle) {
			m_rec_props.packetsize = m_play_props.packetsize =
				format.packet_samps;
		}

		m_format = format;
		return true;
	}

	static snd_pcm_format_t AlsaPcmFormat(sio_sampletype_t st) {
		switch (st) {
		case SIO_PCM_U8:
			return SND_PCM_FORMAT_U8;
		case SIO_PCM_S16_LE:
			return SND_PCM_FORMAT_S16_LE;
		case SIO_PCM_A_LAW:
			return SND_PCM_FORMAT_A_LAW;
		case SIO_PCM_MU_LAW:
			return SND_PCM_FORMAT_MU_LAW;
		default:
			return SND_PCM_FORMAT_UNKNOWN;
		}
	}

	bool SetAvailMin(snd_pcm_t *pcmp, snd_pcm_uframes_t amin) {
		int err;
		snd_pcm_sw_params_t *swp;
		OpLatencyMonitor lat(m_ei, "ALSA SetAvailMin");

		snd_pcm_sw_params_alloca(&swp);

		err = snd_pcm_sw_params_current(pcmp, swp);
		if (err < 0) {
			
			return false;
		}

		err = snd_pcm_sw_params_set_avail_min(pcmp, swp, amin);
		if (err < 0) {
			
			return false;
		}

		err = snd_pcm_sw_params(pcmp, swp);
		if (err < 0) {
			
			return false;
		}

		return true;
	}

	bool ConfigurePcm(snd_pcm_t *pcmp, SoundIoFormat &format,
			  snd_pcm_access_t pcm_access,
			  AlsaChannelProps &props, ErrorInfo *error = 0) {
		snd_pcm_hw_params_t *hwp;
		snd_pcm_sw_params_t *swp;
		snd_pcm_uframes_t buffersize, packetsize, amin;
		snd_pcm_format_t sampfmt;
		int err;

		sampfmt = AlsaPcmFormat(format.sampletype);
		if (sampfmt == SND_PCM_FORMAT_UNKNOWN) {
			
			return false;
		}

		snd_pcm_hw_params_alloca(&hwp);
		snd_pcm_sw_params_alloca(&swp);

		err = snd_pcm_hw_params_any(pcmp, hwp);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_hw_params_set_access(pcmp, hwp, pcm_access);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_hw_params_set_format(pcmp, hwp, sampfmt);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_hw_params_set_rate(pcmp, hwp,
						 format.samplerate, 0);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_hw_params_set_channels(pcmp, hwp,
						     format.nchannels);
		if (err < 0) {
			
			goto failed;
		}

		packetsize = format.packet_samps;

		err = snd_pcm_hw_params_set_period_size_near(pcmp, hwp,
							     &packetsize, 0);
		if (err < 0) {
			
			goto failed;
		}

		buffersize = props.bufsize ? props.bufsize : 0;
		if (buffersize < packetsize)
			
			buffersize = packetsize * 16;

		err = snd_pcm_hw_params_set_buffer_size_near(pcmp, hwp,
							     &buffersize);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_hw_params(pcmp, hwp);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_hw_params_current(pcmp, hwp);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_hw_params_get_buffer_size(hwp, &buffersize);
		if (err < 0) {
			
			goto failed;
		}
		err = snd_pcm_hw_params_get_period_size(hwp, &packetsize, 0);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_sw_params_current(pcmp, swp);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_sw_params_get_avail_min(swp, &amin);
		if (err < 0) {
			
			goto failed;
		}

		err = snd_pcm_prepare(pcmp);
		if (err < 0) {
			
			goto failed;
		}

		props.packetsize = packetsize;
		props.bufsize = buffersize;
		props.min_avail = amin;
		return true;

	failed:
		return false;
	}

	bool Prepare(bool playback, bool capture, ErrorInfo */*error*/) {
		if (playback) {
			assert(m_play_handle);
			
		}
		if (capture) {
			assert(m_rec_handle);
			
			
		}
		return true;
	}

	bool CreateNotifiers(snd_pcm_t *pcm_handle,
			     DispatchInterface *eip,
			     Callback<void, SocketNotifier*, int> &cbtemplate,
			     SocketNotifier **&table, int &count,
			     ErrorInfo *error) {
		struct pollfd *polldesc;
		int i, j, nfds, nnot;
		int err;

		nfds = snd_pcm_poll_descriptors_count(pcm_handle);
		polldesc = (struct pollfd*) alloca(nfds * sizeof(*polldesc));
		err = snd_pcm_poll_descriptors(pcm_handle, polldesc, nfds);
		if (err < 0) {
			
			return false;
		}

		
		nnot = 0;
		for (i = 0; i < nfds; i++) {
			if (polldesc[i].events & POLLIN)
				nnot++;
			if (polldesc[i].events & POLLOUT)
				nnot++;
		}

		table = (SocketNotifier**)
			malloc(nnot * sizeof(SocketNotifier*));

		for (i = 0, j = 0; i < nfds; i++) {
			if (polldesc[i].events & POLLIN) {
				assert(j < nnot);
				table[j] = eip->NewSocket(polldesc[i].fd,
							  false);
				table[j]->Register(cbtemplate);
				j++;
			}
			if (polldesc[i].events & POLLOUT) {
				assert(j < nnot);
				table[j] = eip->NewSocket(polldesc[i].fd,
							  true);
				table[j]->Register(cbtemplate);
				j++;
			}
		}
		assert(j == nnot);
		count = nnot;
		return true;
	}

	bool HasNotifiers(void) const {
		return (m_play_not != 0) || (m_rec_not != 0);
	}

	static void CleanupNotifiers(SocketNotifier **&table, int &count) {
		int i;
		if (count) {
			for (i = 0; i < count; i++) {
				delete table[i];
			}
			free(table);
			table = NULL;
			count = 0;
		}
	}

	
	bool CheckNotifications(void) {
		struct pollfd *pollfds;
		unsigned short revents;
		int ndesc = 0, nused = 0, play_first;
		OpLatencyMonitor lat(m_ei, "ALSA CheckNotifications");

		if (m_rec_handle)
			ndesc = snd_pcm_poll_descriptors_count(m_rec_handle);
		if (m_play_handle)
			ndesc += snd_pcm_poll_descriptors_count(m_play_handle);
		if (!ndesc)
			return false;
		pollfds = (struct pollfd*) alloca(ndesc * sizeof(*pollfds));
		if (m_rec_handle) {
			nused = snd_pcm_poll_descriptors(m_rec_handle,
							 pollfds, ndesc);
			assert(nused <= ndesc);
		}
		play_first = nused;
		if (m_play_handle) {
			nused += snd_pcm_poll_descriptors(m_play_handle,
							  &pollfds[nused],
							  ndesc - nused);
			assert(nused <= ndesc);
		}
		if (!nused)
			return false;
		poll(pollfds, nused, 0);
		if (m_rec_handle && play_first) {
			if (snd_pcm_poll_descriptors_revents(m_rec_handle,
							     pollfds,
							     play_first,
							     &revents) < 0) {
			
			}
			else if (revents & POLLIN)
				return true;
		}
		if (m_play_handle && (nused - play_first)) {
			if (snd_pcm_poll_descriptors_revents(m_play_handle,
						     &pollfds[play_first],
						     nused - play_first,
						     &revents) < 0) {
			
			}
			else if (revents & POLLOUT)
				return true;
		}

		return false;
	}

	bool CheckXrun(snd_pcm_t *handle, bool &xrun, ErrorInfo *error) {
		bool *mxrun;
		if (handle == m_play_handle)
			mxrun = &m_play_xrun;
		else if (handle == m_rec_handle)
			mxrun = &m_rec_xrun;
		else
			abort();
		xrun = *mxrun;
		if (!xrun) {
			xrun = (snd_pcm_state(handle) == SND_PCM_STATE_XRUN);
			if (xrun && !HandleInterruption(handle, -EPIPE, error))
				return false;
		}
		*mxrun = false;
		return true;
	}

	snd_pcm_sframes_t GetPlaybackQueue(void) {
		int err;
		snd_pcm_sframes_t exp;
		assert(m_play_handle);
		err = snd_pcm_delay(m_play_handle, &exp);
		if (err < 0) {
			if (err != -EPIPE)
				
			
			exp = 0;
		}
		if (exp < 0) {
			
			
			exp = 0;
		}
		return exp;
	}

	bool HandleInterruption(snd_pcm_t *pcmp, snd_pcm_sframes_t /*res*/,
				ErrorInfo *error) {
		int err;
		const char *streamtype;
		bool *xrun;

		if (snd_pcm_stream(pcmp) == SND_PCM_STREAM_PLAYBACK) {
			
			xrun = &m_play_xrun;
		} else {
			
			xrun = &m_rec_xrun;
		}

	restart:
		switch (snd_pcm_state(pcmp)) {
		case SND_PCM_STATE_XRUN:
			
			*xrun = true;

		case SND_PCM_STATE_SETUP:
			err = snd_pcm_prepare(pcmp);
			if (err < 0) {
				
				return false;
			}

			assert(snd_pcm_state(pcmp) == SND_PCM_STATE_PREPARED);
			

		case SND_PCM_STATE_PREPARED:
			if (snd_pcm_stream(pcmp) == SND_PCM_STREAM_CAPTURE) {
				err = snd_pcm_start(pcmp);
				if (err < 0) {
					
					return false;
				}
			}
			return true;

		case SND_PCM_STATE_SUSPENDED:
			

			err = snd_pcm_resume(pcmp);
			if (err < 0) {
				
				return false;
			}
			if (snd_pcm_state(pcmp) == SND_PCM_STATE_SUSPENDED) {
				
				return false;
			}
			goto restart;

		case SND_PCM_STATE_RUNNING:
			return true;

		case SND_PCM_STATE_DISCONNECTED:
			
			return false;

		default:
			
			return false;
		}
	}
}