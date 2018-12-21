/*

 */

#if !defined(__MOCKBTPHONEHUB_H__)
#define __MOCKBTPHONEHUB_H__

#include <string>
#include "gmock/gmock.h"
#include "btphoneservice.h"




namespace btphone {

class Mockbtphoneservice : public btphoneservice {
 public:
  MOCK_METHOD1(AddSession,
      void(btphoneinstance *sessp));

  MOCK_METHOD1(RemoveSession,
      void(btphoneinstance *sessp));

  MOCK_CONST_METHOD1(FindSession,
      btphoneinstance*(btphoneclient const *devp));

  MOCK_METHOD1(Start,
      bool(ErrorInfo *error));

  MOCK_METHOD0(Stop,
      void(void));

  MOCK_CONST_METHOD0(GetHub,
      btphonehub*(void));

  MOCK_CONST_METHOD0(GetDi,
      DispatchInterface*(void));

  MOCK_CONST_METHOD0(GetFirstSession,
      btphoneinstance*(void));

  MOCK_CONST_METHOD1(GetNextSession,
      btphoneinstance*(btphoneinstance *));
};

}
#endif


