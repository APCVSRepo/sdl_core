/*

 */

#if !defined(__MOCKBTPHONECLIENT_H__)
#define __MOCKBTPHONECLIENT_H__

#include <string>
#include "gmock/gmock.h"
#include "btphoneclient.h"




namespace btphone {

class Mockbtphoneclient : public btphoneclient {
 public:
  MOCK_METHOD1(NameResolutionResult,
      void(jobhci *taskp));

  MOCK_METHOD1(AddSession,
      void(btphoneinstance *sessp));

  MOCK_METHOD1(RemoveSession,
      void(btphoneinstance *sessp));

  MOCK_CONST_METHOD1(FindSession,
      btphoneinstance*(btphoneservice const *svcp));

  MOCK_METHOD0(DisconnectAll,
      void(void));

  MOCK_CONST_METHOD0(GetAddr,
      const bdaddr_t&(void));

  MOCK_CONST_METHOD1(GetAddr,
      void(char (&namebuf)[32]));

  MOCK_CONST_METHOD0(IsNameResolving,
      bool(void));

  MOCK_CONST_METHOD0(IsNameResolved,
      bool(void));

  MOCK_METHOD1(ResolveName,
      bool(ErrorInfo));

  MOCK_CONST_METHOD0(GetDeviceClass,
      uint32_t(void));
};


}
#endif


