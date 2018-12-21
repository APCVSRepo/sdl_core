/*

 */

#if !defined(__BTPHONE_H__)
#define __BTPHONE_H__

#include <sys/types.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>


#include <btphonecommon.h>
#include <btphonehci.h>
#include <btphonegroup.h>
#include <btphoneclient.h>
#include <btphoneservice.h>


namespace btphone {


class btphonehub {
        friend class sdpjob_fun;
        friend class btphonehci;
        friend class btphonegroup;
        friend class btphoneclient;

private:
	sdp_session_t			*m_sdp;
	DispatchInterface		*m_ei;
        jobhci				*m_inquiry_task;

	ListItem			m_dead_objs;

	ListItem			m_devices;
	ListItem			m_services;

	bool SdpRegister(uint8_t channel);

        sdpjob_fun		m_sdp_handler;
        jobhci				*m_hci;

	int				m_hci_seqid;


        void HciInquiryResult(jobhci *taskp);
	void ClearInquiryFlags(void);

        btphoneclient *FindClientDev(bdaddr_t const &bdaddr);
        btphoneclient *CreateClientDev(bdaddr_t const &bdaddr);
        void DeadObject(btphonegroup *objp);
        void UnreferencedClientDev(btphoneclient *devp);

	void SdpConnectionLost(SocketNotifier *, int fh);
	void __Stop(void);
	void InvoluntaryStop(ErrorInfo *reason);

	SocketNotifier			*m_sdp_not;
	TimerNotifier			*m_timer;

	void Timeout(TimerNotifier*);


	bool			m_autorestart;
	int			m_autorestart_timeout;
	bool			m_autorestart_set;
	bool			m_cleanup_set;

public:
	DispatchInterface *GetDi(void) const { return m_ei; }

        bool SdpTaskSubmit(jobsdp *taskp, ErrorInfo *error);
        void SdpTaskCancel(jobsdp *taskp);

	bool SdpRecordRegister(sdp_record_t *recp, ErrorInfo *error);
	void SdpRecordUnregister(sdp_record_t *recp);


	Callback<void, ErrorInfo*>			cb_NotifySystemState;

        Callback<btphoneclient*, bdaddr_t const &>		cb_BtDeviceFactory;



        Callback<void, btphoneclient *, ErrorInfo*>		cb_InquiryResult;


        btphonehub(DispatchInterface *eip);


        ~btphonehub();


        btphonehci *GetHci(void) const { return m_hci; }

        btphoneclient *DefaultDevFactory(bdaddr_t const &);


        bool AddService(btphoneservice *svcp, ErrorInfo *error = 0);


        void RemoveService(btphoneservice *svcp);


	bool Start(ErrorInfo *error = 0);


	void Stop(void);


	bool IsStarted(void) { return (m_sdp != NULL); }


	bool GetAutoRestart(void) const { return m_autorestart; }


	void SetAutoRestart(bool autostart);


        btphoneclient *GetDevice(bdaddr_t const &raddr, bool create = true);


        btphoneclient *GetDevice(const char *raddr, bool create = true);



        btphoneclient *GetFirstDevice(void);


        btphoneclient *GetNextDevice(btphoneclient *devp);


	bool StartInquiry(int timeout_ms = 5000, ErrorInfo *error = 0);


	void StopInquiry(void);


	bool IsScanning(void) const { return m_inquiry_task != 0; }
};





}
#endif
