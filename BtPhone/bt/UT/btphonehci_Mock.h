/*

 */

#if !defined(__MOCKBTPHONEHUB_H__)
#define __MOCKBTPHONEHUB_H__

#include <string>
#include "gmock/gmock.h"
#include "btphonehci.h"




namespace btphone {

class Mockbtphonehci : public btphonehci {
 
public:
  MOCK_METHOD2(HciSetStatus,
      void(jobhci *taskp, int hcistatus));

  MOCK_METHOD2(HciDataReadyNot,
      void(SocketNotifier *, int fh));

  MOCK_METHOD5(HciSend,
      bool(int fh, jobhci *paramsp, void *data, size_t len, ErrorInfo *error));

  MOCK_METHOD3(HciSubmit,
      bool(int fh, jobhci *paramsp, ErrorInfo *error));

  MOCK_METHOD1(HciResubmit,
      void(TimerNotifier *notp));

  MOCK_METHOD2(HciInit,
      bool(int hci_id, ErrorInfo *error));

  MOCK_METHOD0(HciShutdown,
      void(void));

  MOCK_CONST_METHOD0(GetAddr,
      const bdaddr_t&(void));

  MOCK_METHOD2(Queue,
      bool(jobhci, ErrorInfo));

  MOCK_METHOD1(Cancel,
      void(jobhci *taskp));

  MOCK_METHOD3(GetScoMtu,
      bool(uint16_t, uint16_t, ErrorInfo));

  MOCK_METHOD3(SetScoMtu,
      bool(uint16_t, uint16_t, ErrorInfo));

  MOCK_METHOD2(GetScoVoiceSetting,
      bool(uint16_t, ErrorInfo));

  MOCK_METHOD2(SetScoVoiceSetting,
      bool(uint16_t, ErrorInfo));

  MOCK_METHOD1(GetDeviceClassLocal,
      bool(uint32_t &devclass));

  MOCK_METHOD1(SetDeviceClassLocal,
      bool(uint32_t devclass));
};


}
#endif


