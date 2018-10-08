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

#include "sdpjob_fun_Mock.h"
using ::testing::AtLeast;
using ::testing::Return;


namespace btphone {





int sdpjob_fun::SdpLookupChannel(fieldsjobsdp &htp)
{
	sdp_session_t *sdp;
	sdp_list_t *srch = NULL, *attrs = NULL, *rsp = NULL;
	uuid_t svclass;
	uint16_t attr, attr2;
	int res, chan, brsf;
	int status = ENOENT;

	sdp = sdp_connect(BDADDR_ANY, &htp.m_bdaddr, SDP_RETRY_IF_BUSY);
        Mockbtphonehub oMockhub;
        EXPECT_CALL(oMockhub, SdpConnectionLost()).Times(AtLeast(1));
	if (sdp == NULL) {
		return errno;
	}

	sdp_uuid16_create(&svclass, htp.m_svclass_id);
	srch = sdp_list_append(NULL, &svclass);
	attr = SDP_ATTR_PROTO_DESC_LIST;
	attrs = sdp_list_append(NULL, &attr);
	attr2 = SDP_ATTR_SUPPORTED_FEATURES;
	attrs = sdp_list_append(attrs, &attr2);

	res = sdp_service_search_attr_req(sdp, srch,
					  SDP_ATTR_REQ_INDIVIDUAL,
					  attrs, &rsp);

	if (res != 0) {
		status = errno;

	}

	for (; rsp && status; rsp = rsp->next) {
		sdp_record_t *rec = (sdp_record_t *) rsp->data;
		sdp_list_t *protos = 0;

		if (!sdp_get_access_protos(rec, &protos) &&
		    ((chan = sdp_get_proto_port(protos, RFCOMM_UUID)) != 0)) {

			htp.m_supported_features_present = false;


			if (!sdp_get_int_attr(rec, SDP_ATTR_SUPPORTED_FEATURES,
					      &brsf)) {
				htp.m_supported_features_present = true;
				htp.m_supported_features = brsf;
			}

			htp.m_channel = chan;
			status = 0;
		}

		sdp_list_free(protos, 0);
	}


	sdp_list_free(srch, 0);
	sdp_list_free(attrs, 0);
	if (rsp) { sdp_list_free(rsp, 0); }
	sdp_close(sdp);
	return status;
}


void sdpjob_fun::SdpTaskThread(int rqfd, int rsfd)
{
        fieldsjobsdp itask;
	ssize_t res;


	signal(SIGINT, SIG_IGN);

	while (1) {
		res = read(rqfd, &itask, sizeof(itask));
		if (res < 0) {
			if ((errno == EINTR) ||
			    (errno == ENOMEM) ||
			    (errno == ENOBUFS))
				continue;
			break;
		}
			
		if (res != sizeof(itask))
			break;

		switch (itask.m_tasktype) {
                case fieldsjobsdp::ST_SDP_LOOKUP:
			itask.m_errno = SdpLookupChannel(itask);
			break;
		default:
			assert(0);
		}

		itask.m_complete = true;
		if (write(rsfd, &itask, sizeof(itask)) != sizeof(itask))
			break;
	}
}




} /* namespace btphone */
