/*

 */

#if !defined(__BTPHONEHCI_H__)
#define __BTPHONEHCI_H__

#include <sys/types.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>



#include <btphonecommon.h>
#include <btphonehub.h>
#include <btphonegroup.h>
#include <btphoneclient.h>
#include <btphoneservice.h>


namespace btphone {



class btphonehci : public btphonegroup {
        friend class btphonehub;

	ListItem		m_links;
	bdaddr_t		m_bdaddr;

	int			m_hci_fh;
	int			m_hci_id;
	SocketNotifier		*m_hci_not;
	TimerNotifier		*m_resubmit;
	ListItem		m_hci_tasks;
	bool			m_resubmit_needed;
	bool			m_resubmit_set;


public:
        virtual void HciSetStatus(jobhci *taskp, int hcistatus);
	virtual void HciDataReadyNot(SocketNotifier *, int fh);
        virtual bool HciSend(int fh, jobhci *paramsp, void *data, size_t len,
		     ErrorInfo *error);
        virtual bool HciSubmit(int fh, jobhci *paramsp, ErrorInfo *error);
	virtual void HciResubmit(TimerNotifier *notp);

	virtual bool HciInit(int hci_id, ErrorInfo *error);
	virtual void HciShutdown(void);

        btphonehci(btphonehub *hubp)
                : btphonegroup(hubp), m_hci_fh(-1), m_hci_id(-1),
		  m_hci_not(0), m_resubmit(0), m_resubmit_set(false) {}


        virtual ~btphonehci() { HciShutdown(); }


	virtual bdaddr_t const &GetAddr(void) const { return m_bdaddr; }


        virtual bool Queue(jobhci *in_task, ErrorInfo *error = 0);


        virtual void Cancel(jobhci *taskp);


	virtual bool GetScoMtu(uint16_t &mtu, uint16_t &pkts, ErrorInfo *error = 0);


	virtual bool SetScoMtu(uint16_t mtu, uint16_t pkts, ErrorInfo *error = 0);


	virtual bool GetScoVoiceSetting(uint16_t &vs, ErrorInfo *error = 0);

	virtual bool SetScoVoiceSetting(uint16_t vs, ErrorInfo *error = 0);

	virtual bool GetDeviceClassLocal(uint32_t &devclass);
	virtual bool SetDeviceClassLocal(uint32_t devclass);
};



}
#endif
