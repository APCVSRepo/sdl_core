/*

 */

#if !defined(__MOCKBTPHONEHUB_H__)
#define __MOCKBTPHONEHUB_H__

#include <string>
#include "gmock/gmock.h"
#include "btphonehub.h"




namespace btphone {

class Mockbtphonehub : public btphonehub {
 
public:
  MOCK_METHOD1(SdpRegister,
      bool(uint8_t channel));

  MOCK_METHOD1(HciInquiryResult,
      void(jobhci *taskp));

  MOCK_METHOD0(ClearInquiryFlags,
      void(void));

  MOCK_METHOD1(FindClientDev,
      btphoneclient*(bdaddr_t const &bdaddr));

  MOCK_METHOD1(CreateClientDev,
      btphoneclient*(bdaddr_t const &bdaddr));

  MOCK_METHOD1(DeadObject,
      void(btphonegroup *objp));

  MOCK_METHOD1(UnreferencedClientDev,
      void(btphoneclient *devp));

  MOCK_METHOD2(SdpConnectionLost,
      void(SocketNotifier *, int fh));

  MOCK_METHOD1(InvoluntaryStop,
      void(ErrorInfo *reason));

  MOCK_METHOD1(Timeout,
      void(TimerNotifier*));

  MOCK_CONST_METHOD0(GetDi,
      DispatchInterface*(void));

  MOCK_METHOD2(SdpTaskSubmit,
      bool(jobsdp *taskp, ErrorInfo *error));

  MOCK_METHOD1(SdpTaskCancel,
      void(jobsdp *taskp));

  MOCK_METHOD2(SdpRecordRegister,
      bool(sdp_record_t *recp, ErrorInfo *error));

  MOCK_METHOD1(SdpRecordUnregister,
      void(sdp_record_t *recp));

  MOCK_METHOD2(AddService,
      bool(btphoneservice, ErrorInfo));

  MOCK_METHOD1(RemoveService,
      void(btphoneservice *svcp));

  MOCK_METHOD1(Start,
      bool(ErrorInfo));

  MOCK_METHOD0(Stop,
      void(void));

  MOCK_METHOD0(IsStarted,
      bool(void));

  MOCK_CONST_METHOD0(GetAutoRestart,
      bool(void));

  MOCK_METHOD1(SetAutoRestart,
      void(bool autostart));

  MOCK_METHOD2(GetDevice,
      btphoneclient*(bdaddr_t, bool));

  MOCK_METHOD2(GetDevice,
      btphoneclient*(char, bool));

  MOCK_METHOD0(GetFirstDevice,
      btphoneclient*(void));

  MOCK_METHOD1(GetNextDevice,
      btphoneclient*(btphoneclient *devp));

  MOCK_METHOD2(StartInquiry,
      bool(int, ErrorInfo));

  MOCK_METHOD0(StopInquiry,
      void(void));

  MOCK_CONST_METHOD0(IsScanning,
      bool(void));
};

}
#endif


