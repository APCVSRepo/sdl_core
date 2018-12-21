/*

 */

#if !defined(__BTPHONESERVICE_H__)
#define __BTPHONESERVICE_H__

#include <sys/types.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>




#include <btphonecommon.h>
#include <btphonehci.h>
#include <btphonegroup.h>
#include <btphoneclient.h>
#include <btphonehub.h>


namespace btphone {


class btphoneservice {
	friend class btphonehub;
	friend class btphoneinstance;

private:
	ListItem		m_links;


	ListItem		m_sessions;
	btphonehub			*m_hub;

public:
	virtual void AddSession(btphoneinstance *sessp);
	virtual void RemoveSession(btphoneinstance *sessp);
	virtual btphoneinstance *FindSession(btphoneclient const *devp) const;

	virtual bool Start(ErrorInfo *error) = 0;
	virtual void Stop(void) = 0;

	btphoneservice(void);
	virtual ~btphoneservice();


	virtual btphonehub *GetHub(void) const { return m_hub; }


	virtual DispatchInterface *GetDi(void) const { return GetHub()->GetDi(); }


	virtual btphoneinstance *GetFirstSession(void) const;


	virtual btphoneinstance *GetNextSession(btphoneinstance *) const;
};





} 
#endif 
