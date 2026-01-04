#ifndef MQTT_PROXY_H_
#define MQTT_PROXY_H_

#include <mutex>
#include <unordered_map>

#include <mqtt/mqtt_client.h>
#include <utils/config.h>

#include <bluetooth/bluetooth_manager.h>
#include <bluetooth/rfcomm/server.h>
#include <bluetooth/rfcomm/client.h>


class CORE_API MqttProxy
{
public:
	MqttProxy(BluetoothManager& btManager, BluetoothServer& btServer, JsonConfig& config);

	~MqttProxy();

	bool setup();

	void publish(const std::string& topic, const std::vector<uint8_t>& payload);

protected:
	void connectTo(const std::string& topic, const std::vector<uint8_t>& payload);
	void disconnectTo(const std::string& topic, const std::vector<uint8_t>& payload);
	void sendTo(const std::string& topic, const std::vector<uint8_t>& payload);
	void removeDevices(const std::string& topic, const std::vector<uint8_t>& payload);
	void connectBenchmarkTest(const std::string& topic, const std::vector<uint8_t>& payload);

	void onClientConnected(int clientId, const std::string& address);
	void onClientDisconnected(int clientId, const std::string& address);

	void onServerConnected(const std::string& address, uint8_t channel);
	void onServerDisconnected(const std::string& address, uint8_t channel);

	void onReceiveClientData(const std::string& address, const uint8_t* data, size_t size);
	void onReceiveServerData(const std::string& address, const uint8_t* data, size_t size);


private:
	bool createAndConnect();

	BluetoothManager& _manager;
	BluetoothServer& _server;
	JsonConfig& _config;

	std::unique_ptr<MqttClientImpl> _mqtt;
	std::mutex _clientIdsMutex;
	std::mutex _clientsMutex;

	// 作为服务断连接的设备
	std::unordered_map<std::string, int> _clientIds;
	// 作为客户端
	std::unordered_map<std::string, std::unique_ptr<BluetoothClient>> _clients;

};

#endif // MQTT_PROXY_H_