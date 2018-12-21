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

#include "btphonehci_Mock.h"
using ::testing::WillOnce;
using ::testing::Return;


namespace btphone {


bool btphonehci::HciSend(int fh, jobhci *taskp, void *data, size_t len, ErrorInfo *error)
{
	uint8_t *buf;
	hci_command_hdr *hdrp;
	ssize_t expect;
	ssize_t ret;
	int es;

	expect = 1 + sizeof(*hdrp) + len;
	buf = (uint8_t *) malloc(expect);
	if (!buf) {
		if (error)
			error->SetNoMem();
		return false;
	}

	buf[0] = HCI_COMMAND_PKT;
	hdrp = (hci_command_hdr *) &buf[1];
	hdrp->opcode = taskp->m_opcode;
	hdrp->plen = len;
	if (len)
		memcpy(hdrp + 1, data, len);

	GetDi()->LogDebug("HCI Submit 0x%04x", hdrp->opcode);

	while (1) {
		ret = send(fh, buf, expect, MSG_NOSIGNAL);
		if ((ret < 0) &&
		    ((errno == EAGAIN) ||
		     (errno == EINTR) ||
		     (errno == ENOMEM) ||
		     (errno == ENOBUFS)))
			continue;

		break;
	}

	free(buf);

	if (ret < 0) {
		es = errno;
		GetDi()->LogError(error,
				  LIBHFP_ERROR_SUBSYS_BT,
				  LIBHFP_ERROR_BT_SYSCALL,
				  "HCI write failed: %s", strerror(es));
		return false;
	}

	if (ret != expect) {
		GetDi()->LogError(error,
				  LIBHFP_ERROR_SUBSYS_BT,
				  LIBHFP_ERROR_BT_SYSCALL,
				  "HCI short write: expected: %zd got: %zd",
				  expect, ret);
		return false;
	}

	return true;
}

bool btphonehci::HciSubmit(int fh, jobhci *taskp, ErrorInfo *error)
{
	switch (taskp->m_tasktype) {
        case jobhci::HT_INQUIRY: {
		inquiry_cp req;
		taskp->m_opcode =
			htobs(cmd_opcode_pack(OGF_LINK_CTL, OCF_INQUIRY));
		req.lap[0] = 0x33;
		req.lap[1] = 0x8b;
		req.lap[2] = 0x9e;
		req.length = (taskp->m_timeout_ms + 1279) / 1280;
		req.num_rsp = 0;
                Mockbtphonehci oMockhci;
                EXPECT_CALL(oMockhci, HciSend(fh, taskp, &req, sizeof(req), error)).WillOnce(testing::Return(false));


	}
        case jobhci::HT_READ_NAME: {
		remote_name_req_cp req;
		taskp->m_opcode =
			htobs(cmd_opcode_pack(OGF_LINK_CTL,
					      OCF_REMOTE_NAME_REQ));
		bacpy(&req.bdaddr, &taskp->m_bdaddr);
		req.pscan_mode = taskp->m_pscan;
		req.pscan_rep_mode = taskp->m_pscan_rep;
		req.clock_offset = taskp->m_clkoff;
                Mockbtphonehci oMockhci;
                EXPECT_CALL(oMockhci, HciSend(fh, taskp, &req, sizeof(req), error)).WillOnce(testing::Return(true));
	}
	default:
		abort();
	}
}



} /* namespace btphone */
