/*
 * Copyright (c) 2013-2014, Ford Motor Company
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

#include <pthread.h>
#include <unistd.h>
#include <iomanip>

#include <libusb/libusb.h>

#include <sstream>

#include "transport_manager/usb/libusb/usb_connection.h"
#include "transport_manager/transport_adapter/transport_adapter_impl.h"

#include "utils/logger.h"

#define TOUCH_EVENT_TEST_SUPPORT

#ifdef TOUCH_EVENT_TEST_SUPPORT
#include "protocol_handler/protocol_payload.h"
#include "protocol_handler/protocol_packet.h"
#include "protocol/service_type.h"
#include "utils/bitstream.h"
#include "utils/debug.h"
#include "json/json.h"


#include <sys/time.h>
#include <time.h>



#define GET_TIMESPEC_DIFF_MS(otv, ntv)      ((ntv.tv_nsec >= otv.tv_nsec)?	\
                                            (((ntv.tv_sec-otv.tv_sec)*1000)+((ntv.tv_nsec-otv.tv_nsec)/1000000)):	\
                                            (((ntv.tv_sec-otv.tv_sec-1)*1000)+(((ntv.tv_nsec+1000000000)-otv.tv_nsec)/1000000)))
                                                   

#define GET_LOCAL_TIME_MS(ntv)				((ntv.tv_sec*1000) +(ntv.tv_nsec/1000000))
#define	TOUCH_EVENT_INFO_MAX_NUMBER		    	2048	
#define TOUCH_EVENT_TYPE_BEGIN				0
#define TOUCH_EVENT_TYPE_MOVE				1
#define TOUCH_EVENT_TYPE_END				2
#define	TOUCH_EVENT_TEST_TIME_MS			(1*1000)

typedef struct 
{
	uint32_t	StartTime;
	uint32_t	EndTime;
	uint32_t	sn;
	int		type;
}TIME_INFO;

typedef struct
{
	int 		nMinDiffTime;
	int	 	nMaxDiffTime;
	int	 	nAvgDiffTime;
	int 	 	nBeginType;
	int 	 	nMoveType;
	int 	  	nEndType;
	int		nLen;
	TIME_INFO	info[TOUCH_EVENT_INFO_MAX_NUMBER];
}TOUCH_EVENT_TEST_INFO;

//bool TouchEventTimeInfo(uint32_t sn, uint32_t timestamp, const std::string &type);
//bool TouchEventTimeParse(const std::string &message)
//bool TouchEventTest(protocol_handler::RawMessagePtr rawMessage);
#endif

namespace transport_manager {
namespace transport_adapter {


bool TouchEventTimeInfo(uint32_t sn, uint32_t timestamp, const std::string &type);
bool TouchEventTimeParse(const std::string &message);
bool TouchEventTest(protocol_handler::RawMessagePtr rawMessage);

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

UsbConnection::UsbConnection(const DeviceUID& device_uid,
                             const ApplicationHandle& app_handle,
                             TransportAdapterController* controller,
                             const UsbHandlerSptr usb_handler,
                             PlatformUsbDevice* device)
    : device_uid_(device_uid)
    , app_handle_(app_handle)
    , controller_(controller)
    , usb_handler_(usb_handler)
    , libusb_device_(device->GetLibusbDevice())
    , device_handle_(device->GetLibusbHandle())
    , in_endpoint_(0)
    , in_endpoint_max_packet_size_(0)
    , out_endpoint_(0)
    , out_endpoint_max_packet_size_(0)
    , in_buffer_(NULL)
    , in_transfer_(NULL)
    , out_transfer_(0)
    , out_messages_()
    , current_out_message_()
    , bytes_sent_(0)
    , disconnecting_(false)
    , waiting_in_transfer_cancel_(false)
    , waiting_out_transfer_cancel_(false) {}

UsbConnection::~UsbConnection() {
  LOG4CXX_TRACE(logger_, "enter with this" << this);
  Finalise();
  libusb_free_transfer(in_transfer_);
  delete[] in_buffer_;
  LOG4CXX_TRACE(logger_, "exit");
}

// Callback for handling income and outcome data from lib_usb
void InTransferCallback(libusb_transfer* transfer) {
  static_cast<UsbConnection*>(transfer->user_data)->OnInTransfer(transfer);
}

void OutTransferCallback(libusb_transfer* transfer) {
  static_cast<UsbConnection*>(transfer->user_data)->OnOutTransfer(transfer);
}

bool UsbConnection::PostInTransfer() {
  LOG4CXX_TRACE(logger_, "enter");
  libusb_fill_bulk_transfer(in_transfer_,
                            device_handle_,
                            in_endpoint_,
                            in_buffer_,
                            in_endpoint_max_packet_size_,
                            InTransferCallback,
                            this,
                            0);
  const int libusb_ret = libusb_submit_transfer(in_transfer_);
  if (LIBUSB_SUCCESS != libusb_ret) {
    LOG4CXX_ERROR(
        logger_,
        "libusb_submit_transfer failed: " << libusb_error_name(libusb_ret));
    LOG4CXX_TRACE(
        logger_,
        "exit with FALSE. Condition: LIBUSB_SUCCESS != libusb_submit_transfer");
    return false;
  }
  LOG4CXX_TRACE(logger_, "exit with TRUE");
  return true;
}

std::string hex_data(const unsigned char* const buffer,
                     const int actual_length) {
  std::ostringstream hexdata;
  for (int i = 0; i < actual_length; ++i) {
    hexdata << " " << std::hex << std::setw(2) << std::setfill('0')
            << int(buffer[i]);
  }
  return hexdata.str();
}

void UsbConnection::OnInTransfer(libusb_transfer* transfer) {
  LOG4CXX_TRACE(logger_, "enter with Libusb_transfer*: " << transfer);
  if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
    LOG4CXX_DEBUG(logger_,
                  "USB incoming transfer, size:"
                      << transfer->actual_length << ", data:"
                      << hex_data(transfer->buffer, transfer->actual_length));
    ::protocol_handler::RawMessagePtr data(new protocol_handler::RawMessage(
        0, 0, in_buffer_, transfer->actual_length));
    controller_->DataReceiveDone(device_uid_, app_handle_, data);
  } else {
    LOG4CXX_ERROR(logger_,
                  "USB incoming transfer failed: "
                      << libusb_error_name(transfer->status));
    controller_->DataReceiveFailed(
        device_uid_, app_handle_, DataReceiveError());
  }
  if (disconnecting_) {
    waiting_in_transfer_cancel_ = false;
  } else {
    if (!PostInTransfer()) {
      LOG4CXX_ERROR(logger_,
                    "USB incoming transfer failed with "
                        << "LIBUSB_TRANSFER_NO_DEVICE. Abort connection.");
      AbortConnection();
    }
  }
  LOG4CXX_TRACE(logger_, "exit");
}

void UsbConnection::PopOutMessage() {
  LOG4CXX_TRACE(logger_, "enter");
  bytes_sent_ = 0;
  if (out_messages_.empty()) {
    current_out_message_.reset();
  } else {
    current_out_message_ = out_messages_.front();
    out_messages_.pop_front();
    PostOutTransfer();
  }
  LOG4CXX_TRACE(logger_, "exit");
}

bool UsbConnection::PostOutTransfer() {
  LOG4CXX_TRACE(logger_, "enter");
  out_transfer_ = libusb_alloc_transfer(0);
  if (0 == out_transfer_) {
    LOG4CXX_ERROR(logger_, "libusb_alloc_transfer failed");
    LOG4CXX_TRACE(logger_, "exit with FALSE. Condition: 0 == out_transfer_");
    return false;
  }

#ifdef TOUCH_EVENT_TEST_SUPPORT
  TouchEventTest(current_out_message_);
#endif

  libusb_fill_bulk_transfer(out_transfer_,
                            device_handle_,
                            out_endpoint_,
                            current_out_message_->data() + bytes_sent_,
                            current_out_message_->data_size() - bytes_sent_,
                            OutTransferCallback,
                            this,
                            0);
  const int libusb_ret = libusb_submit_transfer(out_transfer_);
  if (LIBUSB_SUCCESS != libusb_ret) {
    LOG4CXX_ERROR(
        logger_,
        "libusb_submit_transfer failed: " << libusb_error_name(libusb_ret)
                                          << ". Abort connection.");
    AbortConnection();
    LOG4CXX_TRACE(logger_,
                  "exit with FALSE. Condition: "
                      << "LIBUSB_SUCCESS != libusb_fill_bulk_transfer");
    return false;
  }
  LOG4CXX_TRACE(logger_, "exit with TRUE");
  return true;
}

void UsbConnection::OnOutTransfer(libusb_transfer* transfer) {
  LOG4CXX_TRACE(logger_, "enter with  Libusb_transfer*: " << transfer);
  sync_primitives::AutoLock locker(out_messages_mutex_);
  if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
    bytes_sent_ += transfer->actual_length;
    if (bytes_sent_ == current_out_message_->data_size()) {
      LOG4CXX_DEBUG(
          logger_,
          "USB out transfer, data sent: " << current_out_message_.get());
      controller_->DataSendDone(device_uid_, app_handle_, current_out_message_);
      PopOutMessage();
    }
  } else {
    LOG4CXX_ERROR(
        logger_,
        "USB out transfer failed: " << libusb_error_name(transfer->status));
    controller_->DataSendFailed(
        device_uid_, app_handle_, current_out_message_, DataSendError());
    PopOutMessage();
  }
  if (!current_out_message_.valid()) {
    libusb_free_transfer(transfer);
    out_transfer_ = NULL;
    waiting_out_transfer_cancel_ = false;
  }
  LOG4CXX_TRACE(logger_, "exit");
}

TransportAdapter::Error UsbConnection::SendData(
    ::protocol_handler::RawMessagePtr message) {
  LOG4CXX_TRACE(logger_, "enter with RawMessagePtr: " << message.get());
  if (disconnecting_) {
    LOG4CXX_TRACE(logger_,
                  "exit with TransportAdapter::BAD_STATE. Condition: "
                      << "disconnecting_");
    return TransportAdapter::BAD_STATE;
  }
  sync_primitives::AutoLock locker(out_messages_mutex_);
  if (current_out_message_.valid()) {
    out_messages_.push_back(message);
  } else {
    current_out_message_ = message;
    if (!PostOutTransfer()) {
      controller_->DataSendFailed(
          device_uid_, app_handle_, message, DataSendError());
      LOG4CXX_TRACE(
          logger_,
          "exit with TransportAdapter::FAIL. Condition: !PostOutTransfer()");
      return TransportAdapter::FAIL;
    }
  }
  LOG4CXX_TRACE(logger_, "exit with TransportAdapter::OK.");
  return TransportAdapter::OK;
}

void UsbConnection::Finalise() {
  LOG4CXX_TRACE(logger_, "enter");
  LOG4CXX_DEBUG(logger_, "Finalise USB connection " << device_uid_);
  {
    sync_primitives::AutoLock locker(out_messages_mutex_);
    disconnecting_ = true;
    if (out_transfer_) {
      waiting_out_transfer_cancel_ = true;
      if (LIBUSB_SUCCESS != libusb_cancel_transfer(out_transfer_)) {
        waiting_out_transfer_cancel_ = false;
      }
    }
    if (in_transfer_) {
      waiting_in_transfer_cancel_ = true;
      if (LIBUSB_SUCCESS != libusb_cancel_transfer(in_transfer_)) {
        waiting_in_transfer_cancel_ = false;
      }
    }
    for (std::list<protocol_handler::RawMessagePtr>::iterator it =
             out_messages_.begin();
         it != out_messages_.end();
         it = out_messages_.erase(it)) {
      controller_->DataSendFailed(
          device_uid_, app_handle_, *it, DataSendError());
    }
  }
  while (waiting_in_transfer_cancel_ || waiting_out_transfer_cancel_) {
    pthread_yield();
  }
  LOG4CXX_TRACE(logger_, "exit");
}

void UsbConnection::AbortConnection() {
  LOG4CXX_TRACE(logger_, "enter");
  controller_->ConnectionAborted(
      device_uid_, app_handle_, CommunicationError());
  Disconnect();
  LOG4CXX_TRACE(logger_, "exit");
}

TransportAdapter::Error UsbConnection::Disconnect() {
  Finalise();
  controller_->DisconnectDone(device_uid_, app_handle_);
  return TransportAdapter::OK;
}

bool UsbConnection::Init() {
  LOG4CXX_TRACE(logger_, "enter");
  if (!FindEndpoints()) {
    LOG4CXX_ERROR(logger_, "EndPoints was not found");
    LOG4CXX_TRACE(logger_, "exit with FALSE. Condition: !FindEndpoints()");
    return false;
  }
  in_buffer_ = new unsigned char[in_endpoint_max_packet_size_];
  in_transfer_ = libusb_alloc_transfer(0);
  if (NULL == in_transfer_) {
    LOG4CXX_ERROR(logger_, "libusb_alloc_transfer failed");
    LOG4CXX_TRACE(logger_, "exit with FALSE. Condition: NULL == in_transfer_");
    return false;
  }

  controller_->ConnectDone(device_uid_, app_handle_);
  if (!PostInTransfer()) {
    LOG4CXX_ERROR(logger_, "PostInTransfer failed. Call ConnectionAborted");
    controller_->ConnectionAborted(
        device_uid_, app_handle_, CommunicationError());
    LOG4CXX_TRACE(logger_, "exit with FALSE. Condition: !PostInTransfer()");
    return false;
  }

  LOG4CXX_TRACE(logger_, "exit with TRUE");
  return true;
}

bool UsbConnection::FindEndpoints() {
  LOG4CXX_TRACE(logger_, "enter");
  struct libusb_config_descriptor* config;
  const int libusb_ret =
      libusb_get_active_config_descriptor(libusb_device_, &config);
  if (LIBUSB_SUCCESS != libusb_ret) {
    LOG4CXX_ERROR(logger_,
                  "libusb_get_active_config_descriptor failed: "
                      << libusb_error_name(libusb_ret));
    LOG4CXX_TRACE(logger_,
                  "exit with FALSE. Condition: LIBUSB_SUCCESS != libusb_ret");
    return false;
  }

  bool find_in_endpoint = true;
  bool find_out_endpoint = true;

  for (int i = 0; i < config->bNumInterfaces; ++i) {
    const libusb_interface& interface = config->interface[i];
    for (int i = 0; i < interface.num_altsetting; ++i) {
      const libusb_interface_descriptor& iface_desc = interface.altsetting[i];
      for (int i = 0; i < iface_desc.bNumEndpoints; ++i) {
        const libusb_endpoint_descriptor& endpoint_desc =
            iface_desc.endpoint[i];

        const uint8_t endpoint_dir =
            endpoint_desc.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK;
        if (find_in_endpoint && endpoint_dir == LIBUSB_ENDPOINT_IN) {
          in_endpoint_ = endpoint_desc.bEndpointAddress;
          in_endpoint_max_packet_size_ = endpoint_desc.wMaxPacketSize;
          find_in_endpoint = false;
        } else if (find_out_endpoint && endpoint_dir == LIBUSB_ENDPOINT_OUT) {
          out_endpoint_ = endpoint_desc.bEndpointAddress;
          out_endpoint_max_packet_size_ = endpoint_desc.wMaxPacketSize;
          find_out_endpoint = false;
        }
      }
    }
  }
  libusb_free_config_descriptor(config);

  const bool result = !(find_in_endpoint || find_out_endpoint);
  LOG4CXX_TRACE(logger_, "exit with " << (result ? "TRUE" : "FALSE"));
  return result;
}

#ifdef TOUCH_EVENT_TEST_SUPPORT
bool TouchEventTimeInfo(uint32_t sn, uint32_t timestamp, const std::string &type)
{
	static TOUCH_EVENT_TEST_INFO testInfo;
	static bool flag = false;
	static struct timespec StartTime;
	int diff, sum;
	struct timespec currentTime;
	
	//SDL_DEBUG("Enter ........................");
	
	if(flag == false)
	{
		memset((char *)&testInfo, 0, sizeof(TOUCH_EVENT_TEST_INFO));
		flag = true;
		clock_gettime(CLOCK_REALTIME, &StartTime);
	}

	clock_gettime(CLOCK_REALTIME, &currentTime);

	
	testInfo.info[testInfo.nLen].StartTime = timestamp;
	testInfo.info[testInfo.nLen].EndTime = GET_LOCAL_TIME_MS(currentTime) & 0x7fffffff; 
	testInfo.info[testInfo.nLen].sn = sn;
	//SDL_DEBUG("time(%u, %u)", testInfo.info[testInfo.nLen].StartTime, testInfo.info[testInfo.nLen].EndTime);

	if(type.compare("BEGIN") == 0)
	{
		testInfo.info[testInfo.nLen].type = TOUCH_EVENT_TYPE_BEGIN;
	}
	else if(type.compare("MOVE") == 0)
	{
		testInfo.info[testInfo.nLen].type = TOUCH_EVENT_TYPE_MOVE;
	}
	else if(type.compare("END") == 0)
	{
		testInfo.info[testInfo.nLen].type = TOUCH_EVENT_TYPE_END;
	}
	else 
	{
		SDL_DEBUG("type is error (%s)", type.c_str());
		return false;
	}
	
	testInfo.nLen++;

	if(testInfo.nLen >= TOUCH_EVENT_INFO_MAX_NUMBER)
	{
		SDL_DEBUG("node number >= %d", TOUCH_EVENT_INFO_MAX_NUMBER);
		flag = false;	
	}

	if(GET_TIMESPEC_DIFF_MS(StartTime, currentTime) >= TOUCH_EVENT_TEST_TIME_MS)
	{
		SDL_DEBUG("timeout !!!");
		flag = false;
	}

	if(flag == false)
	{
		sum = 0;
		for(int i = 0; i < testInfo.nLen; i++)
		{
			diff = testInfo.info[i].EndTime - testInfo.info[i].StartTime;

			if(i == 0)
			{
				testInfo.nMinDiffTime = testInfo.nMaxDiffTime = diff;
			}
			else
			{
				if(diff < testInfo.nMinDiffTime)
				{
					testInfo.nMinDiffTime = diff;
				}
				else if(diff > testInfo.nMaxDiffTime)
				{
					testInfo.nMaxDiffTime = diff;
				}
			}
			sum += diff;

			switch(testInfo.info[i].type)
			{
				case TOUCH_EVENT_TYPE_BEGIN:
						testInfo.nBeginType++;
						break;
						
				case TOUCH_EVENT_TYPE_MOVE:
						testInfo.nMoveType++;
						break;
						
				case TOUCH_EVENT_TYPE_END:
						testInfo.nEndType++;
						break;
				default:
						break;
			}

	//		SDL_DEBUG("Touch Event Test: type=%d, sn=%d, diffTimeMS=%d", testInfo.info[i].type, testInfo.info[i].sn, diff);
		}

		testInfo.nAvgDiffTime = sum / testInfo.nLen;
		SDL_DEBUG("Touch Event Test(sum=%d): nMinDiffTime=%d, nMaxDiffTime=%d, nAvgDiffTime=%d, nBeginType=%d, nMoveType=%d, nEndType=%d",
						testInfo.nLen, testInfo.nMinDiffTime, testInfo.nMaxDiffTime, testInfo.nAvgDiffTime, testInfo.nBeginType, testInfo.nMoveType, testInfo.nEndType);
	}
	
	return true;
}

bool TouchEventTimeParse(const std::string &message)
{       
        Json::Reader reader;
        Json::Value root;
        std::string time;
        
        //SDL_DEBUG("Enter ........................");
        
        if (reader.parse(message, root) == false)
        {       
                SDL_DEBUG("json format is error!");
                return false;
        }
        
        std::string stype = root["type"].asString();
        
        const Json::Value arrayObj = root["event"];
        
        //SDL_DEBUG("type=%s", stype.c_str());
//      return 0;
        
        if(arrayObj.size() != 1)
        {       
                //SDL_DEBUG("event value format is error! (%zd)", arrayObj.size());
                return false;
        }
        
        uint32_t timestamp = 0;
	uint32_t sn = 0;
        
        for (uint32_t i=0; i<arrayObj.size(); i++)
        {       
                //int n = arrayObj[i]["time"].size();
                timestamp = arrayObj[i]["time"][0].asUInt();
                //SDL_DEBUG("timestamp=%u", timestamp);
		sn  = arrayObj[i]["sn"][0].asUInt();
                break;
        }
        
        if(TouchEventTimeInfo(sn, timestamp, stype) == false)
        {       
                SDL_DEBUG("TouchEventTimeInfo was failed !!!");
                return false;
        }

        return true;
}

bool TouchEventTest(protocol_handler::RawMessagePtr rawMessage)
{       
        //SDL_DEBUG("Enter ........................");
        if(rawMessage->data() == NULL
                || rawMessage->data_size() <= 0)
        {       
                SDL_DEBUG("data is error");
                return false;
        }
        
        if(rawMessage->protocol_version() != protocol_handler::MajorProtocolVersion::PROTOCOL_VERSION_4)
        {       
                //SDL_DEBUG("can't handle protocol version(%d)", rawMessage->protocol_version());
                return false;
        }
        
        if(rawMessage->service_type() != protocol_handler::kRpc)
        {       
                //SDL_DEBUG("message is not Rpc service type!");
                return false;
        }
        
        protocol_handler::ProtocolPayloadV2 payload;
        
        protocol_handler::ProtocolPacket packet(rawMessage->connection_key());
        packet.deserializePacket(rawMessage->data(), rawMessage->data_size());
        utils::BitStream message_bytestream(packet.data(), packet.data_size());
        protocol_handler::Extract(&message_bytestream, &payload, packet.data_size());
        
        //SDL_DEBUG("payload >>>>>>>>>>>>>> json=%s", payload.json.c_str());
        
        if(TouchEventTimeParse(payload.json) == false )
        {       
//                SDL_DEBUG("TouchEventTimeParse was failed! ");
                return false;
        }
        
        return true;

}
#endif

}  // namespace transport_adapter
}  // namespace transport_manager
