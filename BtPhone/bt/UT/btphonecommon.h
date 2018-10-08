/*

 */

#if !defined(__BTPHONE_H__)
#define __BTPHONE_H__

#include <sys/types.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>



#include <btphonehub.h>
#include <btphonehci.h>
#include <btphonegroup.h>
#include <btphoneclient.h>
#include <btphoneservice.h>


namespace btphone {


struct fieldsjobsdp {
	enum sdp_tasktype_t {
		ST_SDP_LOOKUP,
	};

	int		m_seqid;
	sdp_tasktype_t	m_tasktype;
	bdaddr_t	m_bdaddr;
	uint16_t	m_svclass_id;
	int		m_timeout_ms;

	bool		m_complete;
	int		m_errno;
	bool		m_supported_features_present;
	uint8_t		m_channel;
	uint16_t	m_supported_features;
};

struct jobsdp {
	ListItem			m_sdpt_links;
	bool				m_submitted;
	bool				m_resubmit;
        fieldsjobsdp			m_params;
        Callback<void, jobsdp*>	cb_Result;

        jobsdp(void) { memset(&m_params, 0, sizeof(m_params)); }
};



struct jobhci {
	ListItem			m_hcit_links;

	enum hci_tasktype_t {
		HT_INQUIRY,
		HT_READ_NAME,
	};

	hci_tasktype_t			m_tasktype;

	bool				m_complete;
	ErrorInfo			m_error;

	bdaddr_t			m_bdaddr;

	uint32_t			m_devclass;
	uint8_t				m_pscan;
	uint8_t				m_pscan_rep;
	uint16_t			m_clkoff;
	uint16_t			m_opcode;
	int				m_timeout_ms;

	char				m_name[249];

	bool				m_submitted;
	bool				m_resubmit;
        Callback<void, jobhci*>	cb_Result;

        jobhci(void)
		: m_complete(false),
		  m_devclass(0), m_pscan(0), m_pscan_rep(0),
		  m_clkoff(0), m_opcode(0), m_timeout_ms(0),
		  m_submitted(false), m_resubmit(false) {}
};

enum {
        ERR_INVALID = 0,
        ERR_SYSCALL,
        ERR_HCI,
        ERR_NO_SUPPORT,
        ERR_SERVICE_CONFLICT,
        ERR_BAD_SCO_CONFIG,
        ERR_SHUTDOWN,
        ERR_NOT_CONNECTED,
        ERR_NOT_CONNECTED_SCO,
        ERR_ALREADY_STARTED,
        ERR_TIMEOUT,
        ERR_PROTOCOL_VIOLATION,
        ERR_USER_DISCONNECT,
        ERR_COMMAND_ABORTED,
        ERR_COMMAND_REJECTED,
};


}
#endif
