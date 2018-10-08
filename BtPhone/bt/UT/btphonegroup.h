/* -*- C++ -*- */
/*

 */

#if !defined(__BTPHONEGROUP_H__)
#define __BTPHONEGROUP_H__

#include <sys/types.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>



#include <btphonecommon.h>
#include <btphonehci.h>
#include <btphonehub.h>
#include <btphoneclient.h>
#include <btphoneservice.h>

namespace btphone {


class btphonegroup {
        friend class btphonehub;
private:
	ListItem		m_del_links;
        btphonehub			*m_hub;
	int			m_refs;
	void			*m_priv;


public:
	void DeadRemove(void);

        btphonegroup(btphonehub *hubp) : m_hub(hubp), m_refs(1), m_priv(0) {}

        virtual ~btphonegroup(void);


        Callback<void, btphonegroup*> cb_NotifyDestroy;


        virtual btphonehub *GetHub(void) const { return m_hub; }


	virtual DispatchInterface *GetDi(void) const { return m_hub->GetDi(); }


	virtual void *GetPrivate(void) const { return m_priv; }


	virtual void SetPrivate(void *priv) { m_priv = priv; }

	void Get(void) {
		assert(m_refs >= 0);
		if (!m_refs++)
			DeadRemove();
	}


	virtual void Put(void);
};






}
#endif
