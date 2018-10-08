/*

 */

#if !defined(__SDPJOB_FUN_H__)
#define __SDPJOB_FUN_H__

#include <sys/types.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>


#include <btphonehub.h>


namespace btphone {


class sdpjob_fun {
public:
	friend class BtHub;



private:
        class btphonehub			*m_hub;
	DispatchInterface		*m_ei;
	int				m_rqpipe;
	int				m_rspipe;
	pid_t				m_pid;
	SocketNotifier			*m_rspipe_not;
	ListItem			m_tasks;
	bool				m_current_aborted;

public:

	virtual void SdpTaskThread(int rqfd, int rsfd);
        static int SdpLookupChannel(fieldsjobsdp &htp);


	virtual void SdpDataReadyNot(SocketNotifier *, int fh);
	virtual void SdpNextQueue(void);



	virtual bool SdpCreateThread(ErrorInfo *error);
	virtual void SdpShutdown(void);
        virtual bool SdpQueue(jobsdp *in_task, ErrorInfo *error = 0);
        virtual void SdpCancel(jobsdp *taskp);

        sdpjob_fun(btphonehub *hubp, DispatchInterface *eip)
		: m_hub(hubp), m_ei(eip), m_rqpipe(-1), m_rspipe(-1),
		  m_pid(-1), m_rspipe_not(0),
		  m_current_aborted(false) {}

        ~sdpjob_fun() { SdpShutdown(); }
};





}
#endif
