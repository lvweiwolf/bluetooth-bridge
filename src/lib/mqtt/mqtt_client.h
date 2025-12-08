#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mosquittopp.h>

#include <defines.h>
#include <mqtt/job.h>
#include <utils/config.h>

class CORE_API MqttClientImpl : public mosqpp::mosquittopp
{
public:
	using MessageCallback = std::function<void(const std::string&, const std::vector<uint8_t>&)>;
	using ConnectCallback = std::function<void(int)>;
	using DisconnectCallback = std::function<void()>;

	MqttClientImpl(const std::string& client_id = "", bool clean_session = true);

	MqttClientImpl(const std::string& client_id,
				   const std::string& username,
				   const std::string& password,
				   bool clean_session = true);

	~MqttClientImpl();

	// 连接到MQTT代理
	bool connect(const std::string& host, int port = 1883, int keepalive = 60);

	// 断开连接
	void disconnect();

	// 发布消息（异步）
	void publishAsync(const std::string& topic,
					  const std::vector<uint8_t>& payload,
					  int qos = 0,
					  bool retain = false);

	// 订阅主题（异步）
	void subscribeAsync(const std::string& topic, int qos = 0);

	// 取消订阅（异步）
	void unsubscribeAsync(const std::string& topic);

	// 设置消息回调
	void setMessageCallback(const std::string& topic, MessageCallback callback);

	// 设置连接回调
	void setConnectCallback(ConnectCallback callback);

	// 设置断开连接回调
	void setDisconnectCallback(DisconnectCallback callback);

	// 获取连接状态
	bool isConnected() const;

protected:
	// mosquittopp回调函数
	void on_connect(int rc) override;
	void on_disconnect(int rc) override;
	void on_message(const struct mosquitto_message* message) override;
	void on_publish(int mid) override;
	void on_subscribe(int mid, int qos_count, const int* granted_qos) override;
	void on_unsubscribe(int mid) override;

private:
	mutable std::mutex _callback_mutex;
	std::unordered_map<std::string, MessageCallback> _message_callbacks;
	ConnectCallback _connect_callback;
	DisconnectCallback _disconnect_callback;

	std::unique_ptr<JobQueue> _job_queue;
	std::atomic<bool> _connected;
	std::string _client_id;
	std::string _username;
	std::string _password;


	// 内部辅助函数
	void handleConnectAsync(int rc);
	void handleDisconnectAsync(int rc);
	void handleMessageAsync(const std::string& topic, const std::vector<uint8_t>& data);
};


#endif // MQTT_CLIENT_H