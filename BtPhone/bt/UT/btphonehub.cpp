/*

 */



#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <btphonecommon.h>
#include <btphonehub.h>
#include <btphonehci.h>
#include <btphonegroup.h>
#include <btphoneclient.h>
#include <btphoneservice.h>

#include "btphonehub_Mock.h"
using ::testing::WillOnce;
using ::testing::Return;


namespace btphone {


btphoneclient *btphonehub::FindClientDev(bdaddr_t const &bdaddr)
{
        ListItem *listp;
        btphoneclient *devp;
        ListForEach(listp, &m_devices) {
                devp = GetContainer(listp, btphoneclient, m_index_links);
                if (!bacmp(&bdaddr, &devp->m_bdaddr)) {
                        devp->Get();
                        return devp;
                }
        }
        return 0;
}

btphoneclient *btphonehub::CreateClientDev(bdaddr_t const &bdaddr)
{
        btphoneclient *devp;
        char buf[32];

        ba2str(&bdaddr, buf);
        m_ei->LogDebug("Creating record for BDADDR %s", buf);

        /* Call the factory */
        if (cb_btphoneclientFactory.Registered())
                devp = cb_btphoneclientFactory(bdaddr);
        else
                devp = DefaultDevFactory(bdaddr);

        if (devp != NULL) {
                devp->m_hub = this;
                m_devices.AppendItem(devp->m_index_links);
        }
        return devp;
}

btphoneclient *btphonehub::DefaultDevFactory(bdaddr_t const &bdaddr)
{
        return new btphoneclient(this, bdaddr);
}





btphoneclient *btphonehub::GetDevice(bdaddr_t const &raddr, bool create)
{
        btphoneclient *devp;

        Mockbtphonehub oMockhub;
        EXPECT_CALL(oMockhci, FindClientDev(raddr)).WillOnce(testing::ReturnPointee(devp));
        if (!devp && create) {
                devp = CreateClientDev(raddr);
        }
        return devp;
}

btphoneclient *btphonehub::GetDevice(const char *raddr, bool create)
{
        bdaddr_t bdaddr;
        str2ba(raddr, &bdaddr);
        btphoneclient *devp;
        Mockbtphonehub oMockhub;
        EXPECT_CALL(oMockhci, GetDevice(bdaddr, create)).WillOnce(testing::ReturnPointee(devp));
}



} /* namespace btphone */
