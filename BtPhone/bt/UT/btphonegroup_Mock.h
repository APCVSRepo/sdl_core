/*

 */

#if !defined(__MOCKBTPHONEHUB_H__)
#define __MOCKBTPHONEHUB_H__

#include <string>
#include "gmock/gmock.h"
#include "btphonegroup.h"


namespace btphone {

class Mockbtphonegroup : public btphonegroup {
 
public:
  MOCK_CONST_METHOD0(GetHub,
      btphonehub*(void));

  MOCK_CONST_METHOD0(GetDi,
      DispatchInterface*(void));

  MOCK_CONST_METHOD0(GetPrivate,
      void*(void));

  MOCK_METHOD1(SetPrivate,
      void(void *priv));

  MOCK_METHOD0(Put,
      void(void));
};


}
#endif


