#include <mqtt/mqtt_client.h>
#include <spdlog/spdlog.h>

#include <string.h>


//////////////////////////////////////////////////////////////////////////////////
MqttClientImpl::MqttClientImpl(const std::string& client_id, bool clean_session)
	: mosquittopp(client_id.c_str(), clean_session),
	  _job_queue(std::make_unique<JobQueue>(2)), // 使用2个线程处理MQTT操作
	  _connected(false),
	  _client_id(client_id)
{
	// 初始化mosquitto库
	mosqpp::lib_init();

	// 严格的串型模式
    max_inflight_messages_set(0);
}

MqttClientImpl::MqttClientImpl(const std::string& client_id,
					   const std::string& username,
					   const std::string& password,
					   bool clean_session)
	: mosquittopp(client_id.c_str(), clean_session),
	  _job_queue(std::make_unique<JobQueue>(2)),
	  _connected(false),
	  _client_id(client_id)
{
	// 初始化mosquitto库
	mosqpp::lib_init();

	// 严格的串型模式
	max_inflight_messages_set(0);

	// 设置用户名和密码
	username_pw_set(username.c_str(), password.c_str());
}

MqttClientImpl::~MqttClientImpl()
{
	disconnect();
	loop_stop();
	mosqpp::lib_cleanup();
}

bool MqttClientImpl::connect(const std::string& host, int port, int keepalive)
{
	int rc = mosqpp::mosquittopp::connect(host.c_str(), port, keepalive);
	if (rc == MOSQ_ERR_SUCCESS)
	{
		loop_start();
		return true;
	}

	return false;
}

void MqttClientImpl::disconnect() { mosqpp::mosquittopp::disconnect(); }

void MqttClientImpl::publishAsync(const std::string& topic,
							   const std::string& message,
							   int qos,
							   bool retain)
{
	if (!_job_queue)
	{
		return;
	}

	auto payload = static_cast<const void*>(message.data());
	size_t payload_len = message.length();

	// 复制payload数据，确保在异步操作中有效
	std::vector<uint8_t> payload_copy(static_cast<const uint8_t*>(payload),
									  static_cast<const uint8_t*>(payload) + payload_len);

	_job_queue->submit([this, topic, payload_copy, qos, retain]() {
		int rc = mosqpp::mosquittopp::publish(nullptr,
											  topic.c_str(),
											  payload_copy.size(),
											  payload_copy.data(),
											  qos,
											  retain);
		if (rc != MOSQ_ERR_SUCCESS)
			spdlog::error("消息发布失败: {}", mosquitto_strerror(rc));
	});
}

void MqttClientImpl::subscribeAsync(const std::string& topic, int qos)
{
	if (!_job_queue)
	{
		return;
	}

	_job_queue->submit([this, topic, qos]() {
		int rc = mosqpp::mosquittopp::subscribe(nullptr, topic.c_str(), qos);

		if (rc != MOSQ_ERR_SUCCESS)
			spdlog::error("订阅失败: {}", mosquitto_strerror(rc));
	});
}

void MqttClientImpl::unsubscribeAsync(const std::string& topic)
{
	if (!_job_queue)
	{
		return;
	}

	_job_queue->submit([this, topic]() {
		int rc = mosqpp::mosquittopp::unsubscribe(nullptr, topic.c_str());

		if (rc != MOSQ_ERR_SUCCESS)
			spdlog::error("取消订阅失败: {}", mosquitto_strerror(rc));
	});
}

void MqttClientImpl::setMessageCallback(const std::string& topic, MessageCallback callback)
{
	std::lock_guard<std::mutex> lock(_callback_mutex);
	_message_callbacks[topic] = callback;
}

void MqttClientImpl::setConnectCallback(ConnectCallback callback)
{
	std::lock_guard<std::mutex> lock(_callback_mutex);
	_connect_callback = callback;
}

void MqttClientImpl::setDisconnectCallback(DisconnectCallback callback)
{
	std::lock_guard<std::mutex> lock(_callback_mutex);
	_disconnect_callback = callback;
}

bool MqttClientImpl::isConnected() const { return _connected.load(); }

void MqttClientImpl::on_connect(int rc)
{
	_connected.store(true);

	// 异步处理连接回调
	if (_job_queue)
	{
		_job_queue->submit([this, rc]() { handleConnectAsync(rc); });
	}
}

void MqttClientImpl::on_disconnect(int rc)
{
	_connected.store(false);

	// 异步处理断开连接回调
	if (_job_queue)
	{
		_job_queue->submit([this, rc]() { handleDisconnectAsync(rc); });
	}
}

void MqttClientImpl::on_message(const struct mosquitto_message* message)
{
	if (!message || !message->payload)
	{
		return;
	}

	std::string topic(message->topic ? message->topic : "");
	const void* payload = message->payload;
	size_t payload_len = message->payloadlen;

	void* buffer = malloc(payload_len + 1);
	((char*)buffer)[payload_len] = 0;

	if (buffer != nullptr)
		memcpy(buffer, payload, payload_len);

	std::string msg_str = (char*)buffer;

	// 异步处理消息回调
	if (_job_queue)
	{
		_job_queue->submit([this, topic, msg_str]() { handleMessageAsync(topic, msg_str); });
	}
}

void MqttClientImpl::on_publish(int mid)
{
	// 可以在这里添加发布成功的回调处理
}

void MqttClientImpl::on_subscribe(int mid, int qos_count, const int* granted_qos)
{
	// 可以在这里添加订阅成功的回调处理
	spdlog::info("订阅成功");
}

void MqttClientImpl::on_unsubscribe(int mid)
{
	// 可以在这里添加取消订阅成功的回调处理
}

void MqttClientImpl::handleConnectAsync(int rc)
{
	std::lock_guard<std::mutex> lock(_callback_mutex);
	if (_connect_callback)
	{
		_connect_callback(rc);
	}
}

void MqttClientImpl::handleDisconnectAsync(int rc)
{
	std::lock_guard<std::mutex> lock(_callback_mutex);
	if (_disconnect_callback)
	{
		_disconnect_callback();
	}
}

void MqttClientImpl::handleMessageAsync(const std::string& topic, std::string message)
{
	std::lock_guard<std::mutex> lock(_callback_mutex);

	// 查找匹配的回调函数
	for (const auto& pair : _message_callbacks)
	{
		// 简单的字符串匹配，可以扩展为支持通配符
		if (topic.find(pair.first) != std::string::npos)
		{
			if (pair.second)
			{
				pair.second(topic, message);
			}
			break; // 找到第一个匹配的回调就返回
		}
	}
}