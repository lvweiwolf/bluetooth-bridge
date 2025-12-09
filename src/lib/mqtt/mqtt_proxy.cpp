#include <mqtt/mqtt_proxy.h>
#include <utils/base64.h>

#include <mutex>
#include <random>
#include <iomanip>

namespace {

	// 生成UUID字符串的局部函数
	std::string generateUUID()
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, 15);
		std::uniform_int_distribution<> dis2(8, 11);

		std::stringstream ss;

		// UUID格式: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
		for (int i = 0; i < 8; i++)
		{
			ss << std::hex << dis(gen);
		}
		ss << "-";
		for (int i = 0; i < 4; i++)
		{
			ss << std::hex << dis(gen);
		}
		ss << "-4"; // 版本4标识
		for (int i = 0; i < 3; i++)
		{
			ss << std::hex << dis(gen);
		}
		ss << "-";
		ss << std::hex << dis2(gen); // UUID变体标识 (8, 9, a, b)
		for (int i = 0; i < 3; i++)
		{
			ss << std::hex << dis(gen);
		}
		ss << "-";
		for (int i = 0; i < 12; i++)
		{
			ss << std::hex << dis(gen);
		}

		return ss.str();
	}

	std::string formatTime(const std::chrono::system_clock::time_point& tp)
	{
		// 转换为 time_t
		std::time_t tt = std::chrono::system_clock::to_time_t(tp);

		// 转换为本地时间结构
		std::tm* tm = std::localtime(&tt);

		// 格式化为字符串
		std::stringstream ss;
		ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");

		return ss.str();
	}
}

// MqttProxy类的实现
//////////////////////////////////////////////////////////////////
MqttProxy::MqttProxy(BluetoothManager& btManager, BluetoothServer& btServer, JsonConfig& config)
	: _manager(btManager), _server(btServer), _config(config)
{
}

MqttProxy::~MqttProxy() {}

bool MqttProxy::createAndConnect()
{
	// MQTT配置参数
	std::string cliendId = generateUUID();
	std::string username = _config.getString("mqtt.username", "admin");
	std::string password = _config.getString("mqtt.password", "123456");
	std::string server = _config.getString("mqtt.host", "127.0.0.1");
	int32_t port = _config.getInt("mqtt.port", 1883);

	// 创建MQTT客户端
	_mqtt = std::make_unique<MqttClientImpl>(cliendId, username, password, true);

	// 设置连接回调
	_mqtt->setConnectCallback([](int rc) {
		spdlog::info("MQTT连接回调 - 返回码: {}", rc);

		if (rc == 0)
			spdlog::info("MQTT连接成功!");
		else
			spdlog::error("MQTT连接失败，错误码: {}", rc);
	});

	// 设置断开连接回调
	_mqtt->setDisconnectCallback([]() { spdlog::warn("MQTT已断开连接"); });

	// 连接到MQTT代理
	if (!_mqtt->connect(server, port, 60))
	{
		spdlog::error("无法连接到MQTT代理");
		return false;
	}

	return true;
}


bool MqttProxy::setup()
{
	if (!createAndConnect())
		return false;

	// ==== 初始化MQTT订阅和发布 =====
	_server.setClientConnectCallback(std::bind(&MqttProxy::onClientConnected,
											   this,
											   std::placeholders::_1,
											   std::placeholders::_2));

	_server.setClientDisconnectCallback(std::bind(&MqttProxy::onClientDisconnected,
												  this,
												  std::placeholders::_1,
												  std::placeholders::_2));

	_server.setDataReceivedCallback(std::bind(&MqttProxy::onReceiveClientData,
											  this,
											  std::placeholders::_1,
											  std::placeholders::_2,
											  std::placeholders::_3));


	// 连接到设备(客户端连接)
	std::string topic = "/org/booway/bluetooth/connectDevice";
	_mqtt->subscribeAsync(topic, 0);
	_mqtt->setMessageCallback(
		topic,
		std::bind(&MqttProxy::connectTo, this, std::placeholders::_1, std::placeholders::_2));

	// 与设备断开连接(客户端、服务端)
	topic = "/org/booway/bluetooth/disconnectDevice";
	_mqtt->subscribeAsync(topic, 0);
	_mqtt->setMessageCallback(
		topic,
		std::bind(&MqttProxy::disconnectTo, this, std::placeholders::_1, std::placeholders::_2));

	// 处理MQTT发送数据到设备
	topic = "/org/booway/bluetooth/sendToDevice";
	_mqtt->subscribeAsync(topic, 0);
	_mqtt->setMessageCallback(
		topic,
		std::bind(&MqttProxy::sendTo, this, std::placeholders::_1, std::placeholders::_2));

	return true;
}

void MqttProxy::publish(const std::string& topic, const std::vector<uint8_t>& payload)
{
	if (_mqtt)
		_mqtt->publishAsync(topic, payload, 0, false);
}

void MqttProxy::connectTo(const std::string& topic, const std::vector<uint8_t>& payload)
{
	std::string jsonBody(payload.begin(), payload.end());

	// 解析message 为json
	Json::CharReaderBuilder readerBuilder;
	Json::Value root;
	JSONCPP_STRING errs;
	std::istringstream iss(jsonBody);

	if (!Json::parseFromStream(readerBuilder, iss, &root, &errs))
	{
		spdlog::error("解析JSON消息失败: {}", errs);
		return;
	}

	std::string publishId = "";
	std::string publishTime = "";
	int clientConnTimeout = _config.getInt("bluetooth.client.socket_accpet_timeout_ms", 1000);
	int clientRecvTimeout = _config.getInt("bluetooth.client.socket_recv_timeout_ms", 1000);
	int clientBufferSize = _config.getInt("bluetooth.client.socket_buffer_size", 1024);

	auto parseJson = [&](const Json::Value& root, JSONCPP_STRING& lastError) -> bool {
		if (!root.isMember("device"))
		{
			lastError = "JSON解析错误：缺少 'devices' 字段";
			return false;
		}

		Json::Value device = root["device"];

		if (device.isMember("publishId"))
			publishId = device["publishId"].asString();

		if (device.isMember("publishTime"))
			publishTime = device["publishTime"].asString();

		if (!device.isMember("address"))
		{
			lastError = "JSON解析错误: 缺少 'address' 字段";
			return false;
		}

		std::string address = device["address"].asString();

		// 配对和连接到蓝牙
		if (!_manager.requestConnect(address))
		{
			lastError = "设备蓝牙连接失败: " + address;
			return false;
		}

		std::unique_lock<std::mutex> lock(_clientsMutex);
		auto it = _clients.find(address);
		if (it != _clients.end())
		{
			// 已存在客户端，尝试重新连接
			auto& client = it->second;
			if (!client->connect(address, 0))
			{
				lastError = "设备RFCOMM串口连接失败: " + address;
				return false;
			}
		}
		else
		{
			// 创建客户端
			auto client = std::make_unique<BluetoothClient>(address);
			client->setBufferSize(clientBufferSize);
			client->setConnectTimeout(clientConnTimeout);
			client->setRecvTimeout(clientRecvTimeout);
			client->setConnectCallback(std::bind(&MqttProxy::onServerConnected,
												 this,
												 std::placeholders::_1,
												 std::placeholders::_2));

			client->setDisconnectCallback(std::bind(&MqttProxy::onServerDisconnected,
													this,
													std::placeholders::_1,
													std::placeholders::_2));

			client->setDataReceivedCallback(std::bind(&MqttProxy::onReceiveServerData,
													  this,
													  std::placeholders::_1,
													  std::placeholders::_2,
													  std::placeholders::_3));

			if (!client->connect(address, 0))
			{
				lastError = "设备RFCOMM串口连接失败: " + address;
				return false;
			}

			_clients[address] = std::move(client);
		}

		return true;
	};

	if (!parseJson(root, errs))
	{
		Json::Value root;
		root["subscribeId"] = publishId;
		root["subscribeTime"] = publishTime;
		root["message"] = errs;
		std::string body = root.toStyledString();
		std::vector<uint8_t> payload(body.begin(), body.end());

		publish("/org/booway/bluetooth/getLastError", payload);
	}
}

void MqttProxy::disconnectTo(const std::string& topic, const std::vector<uint8_t>& payload)
{
	std::string jsonBody(payload.begin(), payload.end());

	// 解析message 为json
	Json::CharReaderBuilder readerBuilder;
	Json::Value root;
	JSONCPP_STRING errs;
	std::istringstream iss(jsonBody);

	if (!Json::parseFromStream(readerBuilder, iss, &root, &errs))
	{
		spdlog::error("解析JSON消息失败: {}", errs);
		return;
	}

	std::string publishId = "";
	std::string publishTime = "";

	auto parseJson = [&](const Json::Value& root, JSONCPP_STRING& lastError) -> bool {
		if (!root.isMember("device"))
		{
			lastError = "JSON解析错误：缺少 'devices' 字段";
			return false;
		}

		Json::Value device = root["device"];

		if (device.isMember("publishId"))
			publishId = device["publishId"].asString();

		if (device.isMember("publishTime"))
			publishTime = device["publishTime"].asString();

		if (!device.isMember("address"))
		{
			lastError = "JSON解析错误: 缺少 'address' 字段";
			return false;
		}

		std::string address = device["address"].asString();

		{
			// 设备是服务端，断开作为客户端的连接
			auto it = _clients.find(address);
			if (it != _clients.end())
				it->second->disconnect();
		}

		{
			// 设备是客户端，服务端主动断开连接
			auto it = _clientIds.find(address);
			if (it != _clientIds.end())
				_server.disconnectClient(it->second);
		}

		return true;
	};

	if (!parseJson(root, errs))
	{
		Json::Value root;
		root["subscribeId"] = publishId;
		root["subscribeTime"] = publishTime;
		root["message"] = errs;
		std::string body = root.toStyledString();
		std::vector<uint8_t> payload(body.begin(), body.end());

		publish("/org/booway/bluetooth/getLastError", payload);
	}
}

void MqttProxy::sendTo(const std::string& topic, const std::vector<uint8_t>& payload)
{
	std::string jsonBody(payload.begin(), payload.end());

	// 解析message 为json
	Json::CharReaderBuilder readerBuilder;
	Json::Value root;
	JSONCPP_STRING errs;
	std::istringstream iss(jsonBody);

	if (!Json::parseFromStream(readerBuilder, iss, &root, &errs))
	{
		spdlog::error("解析JSON消息失败: {}", errs);
		return;
	}

	std::string publishId = "";
	std::string publishTime = "";

	auto parseJson = [&](const Json::Value& root, JSONCPP_STRING& lastError) -> bool {
		if (!root.isMember("device"))
		{
			lastError = "JSON解析错误：缺少 'devices' 字段";
			return false;
		}

		auto device = root["device"];
		if (device.isMember("publishId"))
			publishId = device["publishId"].asString();

		if (device.isMember("publishTime"))
			publishTime = device["publishTime"].asString();

		if (!device.isMember("address"))
		{
			lastError = "JSON解析错误: 缺少 'address' 字段";
			return false;
		}

		if (!device.isMember("data"))
		{
			lastError = "JSON解析错误: 缺少 'data' 字段";
			return false;
		}

		if (!device.isMember("size"))
		{
			lastError = "JSON解析错误: 缺少 'size' 字段";
			return false;
		}

		std::string address = device["address"].asString();
		std::string strBase64 = device["data"].asString();
		std::string str = base64_decode(strBase64);
		std::vector<uint8_t> data(str.begin(), str.end());

		Json::UInt size = device["size"].asUInt();
		if (size != str.length())
		{
			lastError = "JSON解析错误: 数据校验失败";
			return false;
		}

		{
			// 设备是服务端，发送数据到连接的客户端
			std::lock_guard<std::mutex> lock(_clientIdsMutex);
			auto it = _clientIds.find(address);
			if (it != _clientIds.end())
				_server.sendToClient(it->second, data);
		}

		{
			// 设备是客户端，发送数据到连接的服务端
			std::lock_guard<std::mutex> lock(_clientsMutex);
			auto it = _clients.find(address);
			if (it != _clients.end())
				it->second->send(std::vector<uint8_t>(str.begin(), str.end()));
		}

		return true;
	};

	if (!parseJson(root, errs))
	{
		Json::Value root;
		root["subscribeId"] = publishId;
		root["subscribeTime"] = publishTime;
		root["message"] = errs;
		std::string body = root.toStyledString();
		std::vector<uint8_t> payload(body.begin(), body.end());

		publish("/org/booway/bluetooth/getLastError", payload);
	}
}

#define JSON_BODY_CREATE(x)                                                                        \
	Json::Value root, device;                                                                      \
	device["address"] = (x);                                                                       \
	device["publishId"] = generateUUID();                                                          \
	device["publishTime"] = formatTime(std::chrono::system_clock::now());                          \
	root["device"] = device;                                                                       \
	std::string body = root.toStyledString();

void MqttProxy::onClientConnected(int clientId, const std::string& address)
{
	spdlog::info("客户端已连接: {} - {}", clientId, address);

	{
		std::lock_guard<std::mutex> lock(_clientIdsMutex);
		_clientIds[address] = clientId;
	}

	// 发布客户端连接事件
	JSON_BODY_CREATE(address)
	std::vector<uint8_t> payload(body.begin(), body.end());
	publish("/org/booway/bluetooth/newConnection", payload);
}

void MqttProxy::onClientDisconnected(int clientId, const std::string& address)
{
	spdlog::info("客户端已断开连接: {} - {}", clientId, address);

	{
		std::lock_guard<std::mutex> lock(_clientIdsMutex);
		auto it = _clientIds.find(address);
		if (it != _clientIds.end())
			_clientIds.erase(it);
	}

	// 发布客户端断开连接时间
	JSON_BODY_CREATE(address)
	std::vector<uint8_t> payload(body.begin(), body.end());
	publish("/org/booway/bluetooth/loseConnection", payload);
}

void MqttProxy::onServerConnected(const std::string& address, uint8_t channel)
{
	spdlog::info("服务器已连接: {} - {}", address, static_cast<int>(channel));
	// 发布连接到服务端事件
	JSON_BODY_CREATE(address)
	std::vector<uint8_t> payload(body.begin(), body.end());
	publish("/org/booway/bluetooth/newConnection", payload);
}

void MqttProxy::onServerDisconnected(const std::string& address, uint8_t channel)
{
	spdlog::info("服务器已断开连接: {} - {}", address, static_cast<int>(channel));

	// 保留内存，避免在_clients.erase时析构，导致无法获取远程设备地址
	std::unique_ptr<BluetoothClient> client;

	{
		std::lock_guard<std::mutex> lock(_clientsMutex);
		auto it = _clients.find(address);
		if (it != _clients.end())
		{

			client = std::move(it->second);
			_clients.erase(it);
		}
	}

	// 发布与服务端断开连接事件
	JSON_BODY_CREATE(address)
	std::vector<uint8_t> payload(body.begin(), body.end());
	publish("/org/booway/bluetooth/loseConnection", payload);
}

#define JSON_BODY_DATA_CREATE(x, d, s)                                                             \
	Json::Value root, device;                                                                      \
	device["address"] = (x);                                                                       \
	device["publishId"] = generateUUID();                                                          \
	device["publishTime"] = formatTime(std::chrono::system_clock::now());                          \
	device["data"] = base64_encode((d), (s));                                                      \
	device["size"] = static_cast<Json::UInt>(s);                                                   \
	root["device"] = device;                                                                       \
	std::string body = root.toStyledString();

void MqttProxy::onReceiveClientData(const std::string& address, const uint8_t* data, size_t size)
{
	spdlog::info("收到客户端数据: {} - 大小: {}", address, size);

	JSON_BODY_DATA_CREATE(address, data, size)
	std::vector<uint8_t> payload(body.begin(), body.end());
	publish("/org/booway/bluetooth/receiveFromDevice", payload);
}

void MqttProxy::onReceiveServerData(const std::string& address, const uint8_t* data, size_t size)
{
	spdlog::info("收到服务器数据: {} - 大小: {}", address, size);

	JSON_BODY_DATA_CREATE(address, data, size)
	std::vector<uint8_t> payload(body.begin(), body.end());
	publish("/org/booway/bluetooth/receiveFromDevice", payload);
}
