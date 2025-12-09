#include <chrono>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <mqtt/mqtt_proxy.h>

#include <bluetooth/agent.h>
#include <bluetooth/profile.h>
#include <bluetooth/agent_manager.h>
#include <bluetooth/profile_manager.h>
#include <bluetooth/bluetooth_manager.h>
#include <bluetooth/rfcomm/server.h>
#include <bluetooth/rfcomm/client.h>

#include <utils/config.h>

std::atomic<bool> running(true);


void signalHandler(int signal)
{
	spdlog::info("收到信号: {}, 退出程序。", signal);
	running = false;
}

void setupLogger()
{
	// 控制台sink
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_level(spdlog::level::info);

	// 文件sink
	auto debug_file_sink =
		std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/debug.log", true);
	auto info_file_sink =
		std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/info.log", true);

	debug_file_sink->set_level(spdlog::level::debug);
	info_file_sink->set_level(spdlog::level::info);

	// 创建logger
	std::vector<spdlog::sink_ptr> sinks{ console_sink, debug_file_sink, info_file_sink };
	auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());
	logger->set_level(spdlog::level::debug);

	spdlog::set_default_logger(logger);
	spdlog::flush_on(spdlog::level::warn);
	spdlog::flush_every(std::chrono::seconds(3));
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
			spdlog::error("无法加载配置文件 config.json");
			return 1;
		}

		int publishInterval = config.getInt("bluetooth.publish_interval_ms", 1000);
		int srvAcceptTimeout = config.getInt("bluetooth.server.socket_accpet_timeout_ms", 1000);
		int srvRecvTimeout = config.getInt("bluetooth.server.socket_recv_timeout_ms", 1000);
		int srvBufferSize = config.getInt("bluetooth.server.socket_buffer_size", 1024);
		
		
		// 初始化日志
		setupLogger();

		spdlog::info("===========启动蓝牙设备管理器===========");

		// 1. 设备发现
		auto conn_discovery = sdbus::createSystemBusConnection();
		conn_discovery->enterEventLoopAsync();
		BluetoothManager bluetoothMgr(*conn_discovery);

		// 2.设备配对/连接
		// 2.1 单独创建一个连接用于代理注册和配对处理
		auto conn_agent = sdbus::createSystemBusConnection();
		conn_agent->enterEventLoopAsync();
		AgentManager agent_manager(*conn_agent);

		// 2.2 注册用于配对的代理
		sdbus::ObjectPath agnet_path("/com/example/bluetooth/agent");
		Agent agent(*conn_agent, agnet_path);

		{
			agent.setPinRequestCallback([](const std::string& device_path) {
				spdlog::info("设备 {} 请求PIN码配对: ", device_path);
				std::string pin_code;
				std::cin >> pin_code;
				return pin_code;
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
				Json::Value devicesJson = bluetoothMgr.getPairedDevices();
				std::string body = devicesJson.toStyledString();
				std::vector<uint8_t> payload(body.begin(), body.end());
				
				mqtt.publish("/org/booway/bluetooth/devices", payload);

				ellapse = std::chrono::milliseconds(0);
			}
#if 0
			char response;
			std::cin >> response;

			if (response == 'q' || response == 'Q')
				running = false;

			if (response == 'c' || response == 'C')
			{
				spdlog::info("请输入要连接的设备地址 (格式: XX:XX:XX:XX:XX:XX): ");
				std::string device_address;
				std::cin >> device_address;

				// 客户端连接
				std::string serviceUUID(BluetoothServer::SPP_UUID);
				client.connect(device_address, 0);
			}
#endif
		}

		server.stop();
		conn_discovery->leaveEventLoop();
		conn_agent->leaveEventLoop();
	}
	catch (const std::exception& e)
	{
		spdlog::error("致命错误: {}", e.what());
		return 1;
	}

	return 0;
}