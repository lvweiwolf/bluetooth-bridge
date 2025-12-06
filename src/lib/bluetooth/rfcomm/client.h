#ifndef BLUETOOTH_RFCOMM_CLIENT_H_
#define BLUETOOTH_RFCOMM_CLIENT_H_

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <memory>
#include <string>

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

struct sockaddr_rc;

class BluetoothClient
{
public:
	// 回调函数类型定义
	using ClientCallback = std::function<void(const std::string&, uint8_t)>;
	using DataCallback = std::function<void(const std::string&)>;
	using ErrorCallback = std::function<void(const std::string&)>;
	using StatusCallback = std::function<void(bool connected)>;

	explicit BluetoothClient(const std::string& clientName = "Bluetooth SPP Client");

	~BluetoothClient();

	BluetoothClient(const BluetoothClient&) = delete;
	BluetoothClient(BluetoothClient&&) = delete;
	BluetoothClient& operator=(const BluetoothClient&) = delete;
	BluetoothClient& operator=(BluetoothClient&&) = delete;

	bool connect(const std::string& deviceAddress, uint8_t channel);
	
	void disconnect();

	ssize_t send(const std::string& data);
	
	bool isConnected() const { return _connected; }

	std::string getLocalAddress() const;
	
	std::string getRemoteAddress() const;

	// 设置回调函数
	void setConnectCallback(ClientCallback callback) { _connectCallback = std::move(callback); }

	void setDisconnectCallback(ClientCallback callback)
	{
		_disconnectCallback = std::move(callback);
	}

	void setDataReceivedCallback(DataCallback callback)
	{
		_dataReceivedCallback = std::move(callback);
	}

	void setErrorCallback(ErrorCallback callback)
	{
		_errorCallback = std::move(callback);
	}

	void setStatusCallback(StatusCallback callback)
	{
		_statusCallback = std::move(callback);
	}
	
private:
	// 线程函数
	void receiveThread();
	
	// 内部辅助函数
	bool connectToDevice(const std::string& address, uint8_t channel);
	void cleanupConnection();
	
	// 蓝牙地址转换
	static std::string bdaddrToString(const sockaddr_rc& addr);
	static bool stringTobaddr(const std::string& str, bdaddr_t& addr);
	
	// 错误处理
	void handleError(const std::string& message);

private:
	// 客户端配置
	std::string _clientName;
	int _socket;
	std::atomic<bool> _connected;
	std::atomic<bool> _running;

	// 地址信息
	std::string _localAddress;
	std::string _remoteAddress;
	uint8_t _channel;

	// 线程
	std::thread _receiveThread;

	// 回调函数
	ClientCallback _connectCallback;
	ClientCallback _disconnectCallback;
	DataCallback _dataReceivedCallback;
	ErrorCallback _errorCallback;
	StatusCallback _statusCallback;

	// 互斥锁
	mutable std::mutex _socketMutex;
	mutable std::mutex _callbackMutex;
};

#endif // BLUETOOTH_RFCOMM_CLIENT_H_