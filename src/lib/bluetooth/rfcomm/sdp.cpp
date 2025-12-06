#include <bluetooth/rfcomm/sdp.h>


const bdaddr_t BDADDR_ANY_CC = { { 0, 0, 0, 0, 0, 0 } };
const bdaddr_t BDADDR_LOCAL_CC = { { 0, 0, 0, 0xff, 0xff, 0xff } };

namespace sdp
{
	static int estr2ba(const char* str, bdaddr_t* ba)
	{
		/* Only trap "local", "any" is already dealt with */
		if (!strcmp(str, "local"))
		{
			bacpy(ba, &BDADDR_LOCAL_CC);
			return 0;
		}
		
		return str2ba(str, ba);
	}
	

	bool uuid2str(const uint8_t uuid[16], std::string& strUUID)
	{
		char buffer[37]; // 36个字符 + 1个结束符
		snprintf(buffer,
				 sizeof(buffer),
				 "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
				 uuid[0],
				 uuid[1],
				 uuid[2],
				 uuid[3],
				 uuid[4],
				 uuid[5],
				 uuid[6],
				 uuid[7],
				 uuid[8],
				 uuid[9],
				 uuid[10],
				 uuid[11],
				 uuid[12],
				 uuid[13],
				 uuid[14],
				 uuid[15]);

		strUUID = std::string(buffer);
		return strUUID.length() == 36;
	}

	bool str2uuid(const std::string& strUUID, uint8_t uuid[16])
    {
		// 验证格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
		if (strUUID.length() != 36)
		{
			return false;
		}

		// 验证分隔符位置
		if (strUUID[8] != '-' || strUUID[13] != '-' || strUUID[18] != '-' || strUUID[23] != '-')
		{
			return false;
		}

		// 使用 sscanf 解析
		int result = sscanf(
			strUUID.c_str(),
			"%2hhx%2hhx%2hhx%2hhx-%2hhx%2hhx-%2hhx%2hhx-%2hhx%2hhx-%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
			&uuid[0],
			&uuid[1],
			&uuid[2],
			&uuid[3],
			&uuid[4],
			&uuid[5],
			&uuid[6],
			&uuid[7],
			&uuid[8],
			&uuid[9],
			&uuid[10],
			&uuid[11],
			&uuid[12],
			&uuid[13],
			&uuid[14],
			&uuid[15]);

		return result == 16;
	}

	bool registerSPPService(const std::string& serviceName,
							const std::string& serviceUUID,
							uint8_t rfcommChannel,
							uint32_t& handle)
	{
		const char* svc_dsc = "RFCOMM Serial Port";
		const char* service_prov = "Serial Port Profile Service";

		uuid_t root_uuid, l2cap_uuid, rfcomm_uuid, svc_uuid, svc_class_uuid;
		sdp_list_t *l2cap_list = nullptr, *rfcomm_list = nullptr, *root_list = nullptr,
				   *proto_list = nullptr, *access_proto_list = nullptr, *svc_class_list = nullptr,
				   *profile_list = nullptr;

		sdp_data_t* channel = 0;
		sdp_profile_desc_t profile;
		sdp_record_t sdp_record;
		memset(&sdp_record, 0, sizeof(sdp_record));

		bdaddr_t src, dst;
		bacpy(&src, &BDADDR_ANY_CC);
		bacpy(&dst, &BDADDR_LOCAL_CC);
		sdp_session_t* sdp_session = sdp_connect(&src, &dst, SDP_RETRY_IF_BUSY);

		if (!sdp_session)
		{
			spdlog::error("[SDP] 无法连接到本地SDP服务器");
			return false;
		}

		// 转换128为位UUID
		uint8_t spp_uuid[16] = { 0 };
        str2uuid(serviceUUID, spp_uuid);
		
		// 设置SPP服务ID
		sdp_uuid128_create(&svc_uuid, &spp_uuid);
		sdp_set_service_id(&sdp_record, svc_uuid);

		// 设置服务类列表
		sdp_uuid16_create(&svc_class_uuid, SERIAL_PORT_SVCLASS_ID);
		svc_class_list = sdp_list_append(0, &svc_class_uuid);
		sdp_list_append(svc_class_list, &svc_uuid);
		sdp_set_service_classes(&sdp_record, svc_class_list);

		// 设置蓝牙Profile描述符列表
		sdp_uuid16_create(&profile.uuid, SERIAL_PORT_PROFILE_ID);
		profile.version = 0x0100;
		profile_list = sdp_list_append(0, &profile);
		sdp_set_profile_descs(&sdp_record, profile_list);

		// 设置服务群组
		sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
		root_list = sdp_list_append(0, &root_uuid);
		sdp_set_browse_groups(&sdp_record, root_list);

		// 设置 L2CAP 协议
		sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
		l2cap_list = sdp_list_append(0, &l2cap_uuid);
		proto_list = sdp_list_append(0, l2cap_list);

		// 设置 RFCOMM 协议
		sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
		channel = sdp_data_alloc(SDP_UINT8, &rfcommChannel);
		rfcomm_list = sdp_list_append(0, &rfcomm_uuid);
		sdp_list_append(rfcomm_list, channel);
		sdp_list_append(proto_list, rfcomm_list);

		access_proto_list = sdp_list_append(0, proto_list);
		sdp_set_access_protos(&sdp_record, access_proto_list);

		// 设置服务信息属性
		sdp_set_info_attr(&sdp_record, serviceName.c_str(), service_prov, svc_dsc);

		// 注册服务
		if (sdp_record_register(sdp_session, &sdp_record, SDP_RECORD_PERSIST) == -1)
		{
			spdlog::error("[SDP] 服务注册失败");

			// cleanup
			sdp_data_free(channel);
			sdp_list_free(l2cap_list, nullptr);
			sdp_list_free(rfcomm_list, nullptr);
			sdp_list_free(root_list, nullptr);
			sdp_list_free(access_proto_list, nullptr);
			sdp_list_free(svc_class_list, nullptr);
			sdp_list_free(profile_list, nullptr);
			sdp_close(sdp_session);
			return false;
		}

		handle = sdp_record.handle;

		// cleanup
		sdp_data_free(channel);
		sdp_list_free(l2cap_list, nullptr);
		sdp_list_free(rfcomm_list, nullptr);
		sdp_list_free(root_list, nullptr);
		sdp_list_free(access_proto_list, nullptr);
		sdp_list_free(svc_class_list, nullptr);
		sdp_list_free(profile_list, nullptr);
		sdp_close(sdp_session);
		return true;
	}


	bool unregisterSPPService(uint32_t handle)
	{
		bdaddr_t src, dst;
		bacpy(&src, &BDADDR_ANY_CC);
		bacpy(&dst, &BDADDR_LOCAL_CC);
		sdp_session_t* session = sdp_connect(&src, &dst, SDP_RETRY_IF_BUSY);

		if (!session)
		{
			spdlog::error("[SDP] 无法连接到本地SDP服务器");
			return false;
		}

		uint32_t range = 0x0000ffff;
		sdp_list_t* attrs = nullptr;
		attrs = sdp_list_append(nullptr, &range);
		sdp_record_t* record = sdp_service_attr_req(session, handle, SDP_ATTR_REQ_RANGE, attrs);
		sdp_list_free(attrs, 0);

		if (record)
		{
			if (sdp_device_record_unregister(session, &src, record) == 0)
			{
				sdp_close(session);
				return true;
			}
			else
			{
				spdlog::error("[SDP] 服务注销失败");
				sdp_close(session);
				return false;
			}
		}
		else
		{
			spdlog::error("[SDP] 未找到要注销的服务");
			sdp_close(session);
			return false;
		}
	}

	bool findAvailableSPPChannel(const std::string& address, uint8_t& channel)
	{
		bdaddr_t src, dst;
		bacpy(&src, &BDADDR_ANY_CC);
        estr2ba(address.c_str(), &dst);
		sdp_session_t* sdp_session = sdp_connect(&src, &dst, SDP_RETRY_IF_BUSY);

		if (!sdp_session)
		{
			spdlog::error("[SDP] 无法连接到本地SDP服务器");
			return false;
		}

		channel = 0;
		uuid_t spp_uuid;
		uint32_t range = 0x0000ffff;
		sdp_list_t *response_list = nullptr, *search_list = nullptr, *attrid_list = nullptr;

		// 搜索串口服务
		sdp_uuid16_create(&spp_uuid, SERIAL_PORT_SVCLASS_ID);
		search_list = sdp_list_append(0, &spp_uuid);
		attrid_list = sdp_list_append(0, &range);

		int status = sdp_service_search_attr_req(sdp_session,
												 search_list,
												 SDP_ATTR_REQ_RANGE,
												 attrid_list,
												 &response_list);


		if (status == -1)
		{
			spdlog::error("[SDP] SDP服务搜索失败");
			sdp_list_free(search_list, nullptr);
			sdp_list_free(attrid_list, nullptr);
			sdp_close(sdp_session);
			return false;
		}

		bool find = false;
		sdp_list_t* proto_list = nullptr;
		sdp_list_t* r = response_list;

		for (; r; r = r->next)
		{
			sdp_record_t* record = static_cast<sdp_record_t*>(r->data);

			uuid_t service_uuid;
			char buf[37] = { 0 };
			char name[256] = {0};

			sdp_get_service_id(record, &service_uuid);
			sdp_uuid2strn(&service_uuid, buf, sizeof(buf));
			sdp_get_service_name(record, name, sizeof(name));

			// 转换大写
			std::string service_name(name);
			std::string uuid_str(buf);
			spdlog::info("[Client] 发现服务: {} UUID: {}", service_name, uuid_str);
			
			if (sdp_get_access_protos(record, &proto_list) == 0)
			{
				// get the RFCOMM port number
				channel = sdp_get_proto_port(proto_list, RFCOMM_UUID);
				sdp_list_free(proto_list, 0);
				find = true;
			}
		}

		sdp_list_free(response_list, nullptr);
		sdp_list_free(search_list, nullptr);
		sdp_list_free(attrid_list, nullptr);
		sdp_close(sdp_session);

		return find;
	}
}