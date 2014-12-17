/*
 * Copyright (c) 2013, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_COMPONENTS_FUNCTINAL_MODULE_INCLUDE_FUNCTIONAL_MODULE_FUNCTION_IDS_H_
#define SRC_COMPONENTS_FUNCTINAL_MODULE_INCLUDE_FUNCTIONAL_MODULE_FUNCTION_IDS_H_


namespace functional_modules {

enum MobileFunctionID {
  // Reverse SDL functions ids
  GRANT_ACCESS = 100001,
  CANCEL_ACCESS = 100002,
  START_SCAN = 100003,
  STOP_SCAN = 100004,
  TUNE_RADIO = 100005,
  TUNE_UP = 100006,
  TUNE_DOWN = 100007,
  ON_CONTROL_CHANGED = 100008,
  ON_RADIO_DETAILS = 100009,
  ON_PRESETS_CHANGED = 100010
  /////////////////////////////
};

namespace hmi_api {
  const char grant_access[]       = "VehicleInfo.GrantAccess";
  const char cancel_access[]      = "VehicleInfo.CancelAccess";
  const char on_control_changed[] = "VehicleInfo.OnControlChanged";
}

namespace can_api {
  const char start_scan[]         = "CAN.StartScan";
  const char stop_scan[]          = "CAN.StopScan";
  const char tune_radion[]        = "CAN.TuneRadio";
  const char tune_up[]            = "CAN.TuneUp";
  const char tune_down[]          = "CAN.TuneDown";
  const char on_radio_details[]   = "CAN.OnRadioDetails";
  const char on_preset_changed[]  = "CAN.OnPresetsChanged";
}

}  //  namespace functional_modules

#endif  //  SRC_COMPONENTS_FUNCTINAL_MODULE_INCLUDE_FUNCTIONAL_MODULE_FUNCTION_IDS_H_
