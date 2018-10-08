/*

 */

#if !defined(__BTPHONECLIENT_H__)
#define __BTPHONECLIENT_H__

#include <sys/types.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>



#include <btphonecommon.h>
#include <btphonehci.h>
#include <btphonegroup.h>
#include <btphonehub.h>
#include <btphoneservice.h>


namespace btphone {


class btphoneclient : public btphonegroup {
        friend class btphonehub;
        friend class btphoneservice;
        friend class btphoneinstance;

private:
	ListItem		m_index_links;
	bdaddr_t		m_bdaddr;
	ListItem		m_sessions;
	bool			m_inquiry_found;
	uint16_t		m_inquiry_clkoff;
	uint8_t			m_inquiry_pscan;
	uint8_t			m_inquiry_pscan_rep;
	uint32_t		m_inquiry_class;
	bool			m_name_resolved;
        jobhci			*m_name_task;

	enum { DEVICE_MAX_NAMELEN = 249 };
	char			m_dev_name[DEVICE_MAX_NAMELEN];

public:
        virtual void NameResolutionResult(jobhci *taskp);

        virtual void AddSession(btphoneinstance *sessp);
        virtual void RemoveSession(btphoneinstance *sessp);
        virtual btphoneinstance *FindSession(btphoneservice const *svcp) const;

	void __DisconnectAll(ErrorInfo *reason);



        btphoneclient(btphonehub *hubp, bdaddr_t const &bdaddr);


        virtual ~btphoneclient();


	virtual void DisconnectAll(void) { __DisconnectAll((ErrorInfo *)false); }

        Callback<void, btphoneclient *, const char *, ErrorInfo*>
						cb_NotifyNameResolved;


	const char *GetName(void) const { return m_dev_name; }


	virtual bdaddr_t const &GetAddr(void) const { return m_bdaddr; }


	virtual void GetAddr(char (&namebuf)[32]) const;


	virtual bool IsNameResolving(void) const { return m_name_task != 0; }


	virtual bool IsNameResolved(void) const { return m_name_resolved; }


	virtual bool ResolveName(ErrorInfo *error = 0);

	virtual uint32_t GetDeviceClass(void) const { return m_inquiry_class; }
};





}
#endif
