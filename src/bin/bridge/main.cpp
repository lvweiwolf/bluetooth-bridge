#include <chrono>
#include <atomic>
#include <csignal>

#include <random>
#include <sstream>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <mqtt/mqtt_client.h>

#include <bluetooth/agent.h>
#include <bluetooth/profile.h>
#include <bluetooth/agent_manager.h>
#include <bluetooth/profile_manager.h>
#include <bluetooth/bluetooth_manager.h>
#include <bluetooth/rfcomm/server.h>
#include <bluetooth/rfcomm/client.h>


std::atomic<bool> running(true);

namespace {

	// 生成GUID字符串的局部函数
	std::string generate_guid()
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
}

// MQTT配置和测试的局部函数
std::shared_ptr<MqttClientImpl> create_and_connect_mqtt()
{
	// MQTT配置参数
	std::string cliendId = generate_guid();
	std::string username = "zhgd";
	std::string password = "zhgd@1";
	std::string server = "10.1.7.52";
	int port = 21883;

	// 创建MQTT客户端
	auto mqtt_client = std::make_shared<MqttClientImpl>(cliendId, username, password, true);

	// 设置连接回调
	mqtt_client->setConnectCallback([](int rc) {
		spdlog::info("MQTT连接回调 - 返回码: {}", rc);

		if (rc == 0)
			spdlog::info("MQTT连接成功!");
		else
			spdlog::error("MQTT连接失败，错误码: {}", rc);
	});

	// 设置断开连接回调
	mqtt_client->setDisconnectCallback([]() { spdlog::warn("MQTT已断开连接"); });


	// 连接到MQTT代理
	if (mqtt_client->connect(server, port, 60))
	{
		// 配置订阅
		mqtt_client->subscribeAsync("/bluetooth/paired", 0);
		mqtt_client->setMessageCallback(
			"/bluetooth/paired",
			[](const std::string& topic, std::string message) {
				spdlog::info("收到消息 - 主题: {}, 内容: {}", topic, message);
			});
	}
	else
	{
		spdlog::error("无法连接到MQTT代理");
	}

	return mqtt_client;
}


void signal_handler(int signal)
{
	spdlog::info("收到信号: {}, 退出程序。", signal);
	running = false;
}

void setup_logger()
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
	auto logger = std::make_shared<spdlog::logger>("multi_logger", sinks.begin(), sinks.end());
	logger->set_level(spdlog::level::debug);

	spdlog::set_default_logger(logger);
	spdlog::flush_on(spdlog::level::warn);
	spdlog::flush_every(std::chrono::seconds(3));
}


int main(int argc, char* argv[])
{
	// 设置信号处理
	std::signal(SIGINT, signal_handler);
	std::signal(SIGTERM, signal_handler);

	// 初始化日志
	setup_logger();

	// 调用封装的MQTT配置和测试函数
	auto mqtt_client = create_and_connect_mqtt();

	try
	{
		spdlog::info("启动蓝牙设备管理器");

		// 1. 设备发现
		auto conn_discovery = sdbus::createSystemBusConnection();
		conn_discovery->enterEventLoopAsync();
		BluetoothManager bluetooth_mgr(*conn_discovery);

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
#if 0
			spdlog::info("设备 {} 请求确认配对，Passkey: {}", device_path, passkey);

			char response;
			std::cin >> response;
			return (response == 'y' || response == 'Y');
#else
				// 自动确认配对
				return true;
#endif
			});

			agent_manager.registerAgent(agnet_path, "KeyboardDisplay");
			agent_manager.requestDefaultAgent(agnet_path);
		}

		// 3 启动RFCOMM服务器监听连接
		BluetoothServer server("Bluetooth RFCOMM Server", 0);
		server.start();

		// 4. 启动RFCOMM客户端
		BluetoothClient client;
		client.setConnectCallback([&](const std::string& address, uint8_t channel) {
			spdlog::info("[Client] 客户端连接到服务器 {} (channel {})", address, (int)channel);
			client.send("Hello from Bluetooth Client!");
		});

		std::chrono::milliseconds interval(0);
		auto preTime = std::chrono::steady_clock::now();
		
		// 主循环，等待设备发现
		while (running)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));

			auto curTime = std::chrono::steady_clock::now();
			interval += std::chrono::duration_cast<std::chrono::milliseconds>(curTime - preTime);
			preTime = curTime;

			if (interval.count() >= 3000)
			{
				// MQTT 发布订阅
				Json::Value devicesJson = bluetooth_mgr.getPairedDevices();
				std::string devicesStr = devicesJson.toStyledString();
				mqtt_client->publishAsync("/org/booway/bluetooth/devices", devicesStr, 0, false);

				interval = std::chrono::milliseconds(0);
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
