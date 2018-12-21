/*

 */

#if !defined(__MOCKSDPJOB_FUN_H__)
#define __MOCKSDPJOB_FUN_H__

#include <string>
#include "gmock/gmock.h"
#include "sdpjob_fun.h"




namespace btphone {

class Mocksdpjob_fun : public sdpjob_fun {
 
public:
  MOCK_METHOD2(SdpTaskThread,
      void(int rqfd, int rsfd));

  MOCK_METHOD2(SdpDataReadyNot,
      void(SocketNotifier *, int fh));

  MOCK_METHOD0(SdpNextQueue,
      void(void));

  MOCK_METHOD1(SdpCreateThread,
      bool(ErrorInfo *error));

  MOCK_METHOD0(SdpShutdown,
      void(void));

  MOCK_METHOD2(SdpQueue,
      bool(jobsdp, ErrorInfo));

  MOCK_METHOD1(SdpCancel,
      void(jobsdp *taskp));
};



}
#endif


