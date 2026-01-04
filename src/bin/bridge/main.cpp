#include <chrono>
#include <atomic>
#include <csignal>
#include <cstdint>

#include <mqtt/mqtt_proxy.h>

#include <bluetooth/agent.h>
#include <bluetooth/profile.h>
#include <bluetooth/agent_manager.h>
#include <bluetooth/profile_manager.h>
#include <bluetooth/bluetooth_manager.h>
#include <bluetooth/rfcomm/server.h>
#include <bluetooth/rfcomm/client.h>

#include <utils/config.h>
// 初始化日志
#include <utils/logger.h>

std::atomic<bool> running(true);

void signalHandler(int signal)
{
	LOG_INFO("收到信号 - {}, 退出程序。", signal);
	running = false;
}

int main(int argc, char* argv[])
{
	// 设置信号处理
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);

	try
	{
		// 读取配置文件
		JsonConfig config("config.json");
		if (!config.load())
		{
			LOG_ERROR("无法加载配置文件 config.json");
			return 1;
		}

		// MQTT 发布时间间隔
		int publishInterval = config.getInt("bluetooth.publish_interval_ms", 1000);

		// RFCOMM通信相关参数
		int srvAcceptTimeout = config.getInt("bluetooth.server.socket_accpet_timeout_ms", 1000);
		int srvRecvTimeout = config.getInt("bluetooth.server.socket_recv_timeout_ms", 1000);
		int srvBufferSize = config.getInt("bluetooth.server.socket_buffer_size", 1024);

		// 蓝牙配对/连接相关参数
		int maxRepairCount = config.getInt("bluetooth.max_repair_count", 3);
		int maxReconnectCount = config.getInt("bluetooth.max_reconnect_count", 3);
		int pairTimeout = config.getInt("bluetooth.timeout_pair_ms", 1000);
		int connectTimeout = config.getInt("bluetooth.timeout_connect_ms", 1000);


		LOG_INFO("===========启动蓝牙设备管理器===========");

		auto conn_adapter = sdbus::createSystemBusConnection();
		auto conn_devices = sdbus::createSystemBusConnection();
		auto conn_agent = sdbus::createSystemBusConnection();

		// 1. 设备发现
		BluetoothManager bluetoothMgr(*conn_adapter, *conn_devices);
		bluetoothMgr.setMaxRepairCount(maxRepairCount);
		bluetoothMgr.setMaxReconnectCount(maxReconnectCount);
		bluetoothMgr.setPairTimeout(pairTimeout);
		bluetoothMgr.setConnectTimeout(connectTimeout);

		// 2.设备配对/连接
		// 2.1 单独创建一个连接用于代理注册和配对处理
		AgentManager agent_manager(*conn_agent);

		conn_adapter->enterEventLoopAsync();
		conn_devices->enterEventLoopAsync();
		conn_agent->enterEventLoopAsync();


		// 2.2 注册用于配对的代理
		sdbus::ObjectPath agnet_path("/com/example/bluetooth/agent");
		Agent agent(*conn_agent, agnet_path);

		{
			agent.setPinRequestCallback([&](const std::string& device_path) {
				std::string pincode;
				bluetoothMgr.getPincode(device_path, pincode);
				LOG_INFO("确认PIN码: {}", pincode);
				return pincode;
			});

			agent.setConfirmationCallback([](const std::string& device_path, uint32_t passkey) {
				// 自动确认配对
				return true;
			});

			agent_manager.registerAgent(agnet_path, "KeyboardDisplay");
			agent_manager.requestDefaultAgent(agnet_path);
		}

		// 3 启动RFCOMM服务器监听连接
		BluetoothServer server("Bluetooth RFCOMM Server", 0);
		server.setBufferSize(srvBufferSize);
		server.setAcceptTimeout(srvAcceptTimeout);
		server.setRecvTimeout(srvRecvTimeout);
		server.start();

		// 4. 配置MQTT代理
		MqttProxy mqtt(bluetoothMgr, server, config);
		mqtt.setup();

		std::chrono::milliseconds ellapse(0);
		auto preTime = std::chrono::steady_clock::now();

		// 主循环，等待设备发现
		while (running)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			auto curTime = std::chrono::steady_clock::now();
			ellapse += std::chrono::duration_cast<std::chrono::milliseconds>(curTime - preTime);
			preTime = curTime;

			if (ellapse.count() >= publishInterval)
			{
				// MQTT 发布订阅
				Json::Value adaptersJson = bluetoothMgr.getAdapters();
				std::string body = adaptersJson.toStyledString();
				std::vector<uint8_t> payload(body.begin(), body.end());
				mqtt.publish("/org/booway/bluetooth/getAdapters", payload);

				Json::Value devicesJson = bluetoothMgr.getDevices();
				body = devicesJson.toStyledString();
				payload = std::vector<uint8_t>(body.begin(), body.end());
				mqtt.publish("/org/booway/bluetooth/getDevices", payload);

				ellapse = std::chrono::milliseconds(0);
			}
		}

		server.stop();
		conn_adapter->leaveEventLoop();
		conn_devices->leaveEventLoop();
		conn_agent->leaveEventLoop();
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("致命错误 - {}", e.what());
		return 1;
	}

	return 0;
}