#ifndef BLUETOOTH_RFCOMM_SERVER_H_
#define BLUETOOTH_RFCOMM_SERVER_H_

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <memory>
#include <string>

#include <vector>
#include <unordered_map>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

struct sockaddr_rc;

class BluetoothServer
{
public:
	// 回调函数类型定义
	using ClientCallback = std::function<void(int, const std::string&)>;
	using DataCallback = std::function<void(int, const std::string&)>;
	using ErrorCallback = std::function<void(int, const std::string&)>;

	// SPP服务UUID (00001101-0000-1000-8000-00805F9B34FB)
	static const char* SPP_UUID;

	// 客户端信息结构
	struct ClientInfo
	{
		int socket;
		std::string address;
		std::thread workThread;
		std::atomic<bool> running;
		std::chrono::time_point<std::chrono::steady_clock> connectTime;
	};

	explicit BluetoothServer(const std::string& name = "Bluetooth SPP Server",
							 uint8_t channel = 1);

	~BluetoothServer();

	BluetoothServer(const BluetoothServer&) = delete;
	BluetoothServer(BluetoothServer&&) = delete;
	BluetoothServer& operator=(const BluetoothServer&) = delete;
	BluetoothServer& operator=(BluetoothServer&&) = delete;

	bool start();

	void stop();

	bool isRunning() const { return _running; }

	ssize_t sendToClient(int clientId, const std::string& data);

	size_t broadcast(const std::string& data);

	void disconnectClient(int clientId);

	std::vector<int> getConnectedClients() const;

	std::string getClientInfo(int clientId) const;

	void setClientConnectCallback(ClientCallback callback)
	{
		_clientConnectCallback = std::move(callback);
	}

	void setClientDisconnectCallback(ClientCallback callback)
	{
		_clientDisconnectCallback = std::move(callback);
	}

	void setDataReceivedCallback(DataCallback callback)
	{
		_dataReceivedCallback = std::move(callback);
	}

	void setErrorCallback(ErrorCallback callback) { _errorCallback = std::move(callback); }

	std::string getLocalAddress() const;

	uint8_t getChannel() const { return _channel; }

private:
	void acceptThread();
	void clientThread(int clientId, std::shared_ptr<ClientInfo> clientInfo);
	void cleanupClient(int clientId);
	int getNextClientId();

	// 蓝牙地址转换
	static std::string bdaddrToString(const sockaddr_rc& addr);
	static bool stringTobaddr(const std::string& str, bdaddr_t& addr);

private:
	// 服务器配置
	std::string _serverName;
	uint8_t _channel;
	int _srvSocket;
	std::atomic<bool> _running;

	// 客户端管理
	mutable std::mutex _clientsMutex;
	std::unordered_map<int, std::shared_ptr<ClientInfo>> _clients;
	int _nextClientId;

	// 线程
	std::thread _acceptThread;

	// 回调函数
	ClientCallback _clientConnectCallback;
	ClientCallback _clientDisconnectCallback;
	DataCallback _dataReceivedCallback;
	ErrorCallback _errorCallback;

	uint32_t _sdp_handle;
};


#endif // BLUETOOTH_RFCOMM_SERVER_H_