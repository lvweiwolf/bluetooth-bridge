#ifndef BLUETOOTH_RFCOMM_SDP_H_
#define BLUETOOTH_RFCOMM_SDP_H_

#include <string>

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>



namespace sdp {

	bool uuid2str(const uint8_t uuid[16], std::string& strUUID);

	bool str2uuid(const std::string& strUUID, uint8_t uuid[16]);


	bool registerSPPService(const std::string& serviceName,
							const std::string& serviceUUID,
							uint8_t rfcommChannel,
							uint32_t& handle);

	bool unregisterSPPService(uint32_t handle);

	bool findAvailableSPPChannel(const std::string& address, uint8_t& channel);
}

#endif // BLUETOOTH_RFCOMM_SDP_H_