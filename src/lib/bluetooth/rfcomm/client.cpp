#include <bluetooth/rfcomm/client.h>
#include <bluetooth/rfcomm/sdp.h>

#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>


BluetoothClient::BluetoothClient(const std::string& clientName)
	: _clientName(clientName),
	  _socket(-1),
	  _connected(false),
	  _running(false),
	  _channel(0),
	  _connectCallback(nullptr),
	  _disconnectCallback(nullptr),
	  _dataReceivedCallback(nullptr),
	  _errorCallback(nullptr),
	  _statusCallback(nullptr)
{

	_connectCallback = [](const std::string& address, uint8_t channel) {
		spdlog::info("[Client] 成功连接到服务器 {} (channel {})", address, (int)channel);
	};

	_disconnectCallback = [](const std::string& address, uint8_t channel) {
		spdlog::info("[Client] 从服务器 {} (channel {}) 断开连接", address, (int)channel);
	};

	_dataReceivedCallback = [](const std::string& data) {
		spdlog::info("[Client] 接收到数据: {}", data);
	};

	_errorCallback = [](const std::string& errorMessage) {
		spdlog::error("[Client] 错误: {}", errorMessage);
	};

	_statusCallback = [](bool connected) {
		spdlog::info("[Client] 状态: {}", connected ? "已连接" : "已断开");
	};
}

BluetoothClient::~BluetoothClient()
{
	disconnect();
}

bool BluetoothClient::connect(const std::string& deviceAddress, uint8_t channel)
{
	if (_connected)
	{
		spdlog::error("[Client] 已经连接到设备，先断开现有连接");
		disconnect();
	}

	if (channel == 0)
	{
		// 自动查询可用通道
		if (!sdp::findAvailableSPPChannel(deviceAddress, _channel))
		{
			handleError("查询RFCOMM服务通道失败");
			return false;
		}
	}
	else
	{
		_channel = channel;
	}
	
	_remoteAddress = deviceAddress;

	if (!connectToDevice(deviceAddress, _channel))
	{
		handleError("连接失败");
		return false;
	}

	// 启动接收线程
	_running = true;
	_connected = true;
	_receiveThread = std::thread(&BluetoothClient::receiveThread, this);

	// 获取本地地址
	_localAddress = getLocalAddress();

	// 通知连接成功
	if (_connectCallback)
		_connectCallback(_remoteAddress, _channel);

	if (_statusCallback)
		_statusCallback(true);

	spdlog::info("[Client] 成功连接到设备: {} , channel: {}", deviceAddress, (int)_channel);
	return true;
}

void BluetoothClient::disconnect()
{
	_running = false;
	_connected = false;

	// 等待接收线程结束
	if (_receiveThread.joinable())
		_receiveThread.join();

	cleanupConnection();

	// 通知断开连接
	if (_disconnectCallback)
		_disconnectCallback(_remoteAddress, _channel);

	if (_statusCallback)
		_statusCallback(false);
}

ssize_t BluetoothClient::send(const std::string& data)
{
	if (!_connected || _socket < 0)
	{
		handleError("未连接到设备，无法发送数据");
		return -1;
	}

	std::lock_guard<std::mutex> lock(_socketMutex);
	ssize_t sent = ::send(_socket, data.c_str(), data.length(), 0);
	
	if (sent < 0)
	{
		handleError("发送数据失败: " + std::string(strerror(errno)));
		return -1;
	}

	spdlog::info("[Client] 发送数据 ({} 字节)：{}", sent, data);
	return sent;
}

std::string BluetoothClient::getLocalAddress() const
{
	if (_socket < 0)
	{
		return "未知";
	}

	struct sockaddr_rc localAddr;
	socklen_t len = sizeof(localAddr);
	
	if (getsockname(_socket, (struct sockaddr*)&localAddr, &len) < 0)
	{
		return "获取失败";
	}

	return bdaddrToString(localAddr);
}

std::string BluetoothClient::getRemoteAddress() const
{
	return _remoteAddress;
}

void BluetoothClient::receiveThread()
{
	char buffer[1024];
	
	while (_running && _connected)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(_socket, &readfds);
		
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000; // 100ms超时
		
		int result = select(_socket + 1, &readfds, nullptr, nullptr, &timeout);
		
		if (result < 0)
		{
			if (errno != EINTR)
			{
				handleError("select错误: " + std::string(strerror(errno)));
				break;
			}
			continue;
		}
		
		if (result == 0)
		{
			// 超时，继续循环
			continue;
		}
		
		if (FD_ISSET(_socket, &readfds))
		{
			ssize_t received = recv(_socket, buffer, sizeof(buffer) - 1, 0);
			
			if (received <= 0)
			{
				if (received == 0)
					spdlog::info("[Client] 连接已关闭");
				else
				{
					handleError("接收数据错误: " + std::string(strerror(errno)));
				}
				
				break;
			}
			
			buffer[received] = '\0';
			std::string data(buffer);
			spdlog::info("[Client] 接收数据 ({} 字节): {}", received, data);
			
			// 调用数据接收回调
			if (_dataReceivedCallback)
			{
				std::lock_guard<std::mutex> lock(_callbackMutex);
				_dataReceivedCallback(data);
			}
		}
	}
}

bool BluetoothClient::connectToDevice(const std::string& address, uint8_t channel)
{
	_socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (_socket < 0)
	{
		spdlog::error("[Client] 创建socket失败: {}", strerror(errno));
		return false;
	}

	// 设置非阻塞模式
	int flags = fcntl(_socket, F_GETFL, 0);
	if (flags < 0 || fcntl(_socket, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		spdlog::error("[Client] 设置非阻塞模式失败: {}", strerror(errno));
		close(_socket);
		_socket = -1;
		return false;
	}

	struct sockaddr_rc addr;
	memset(&addr, 0, sizeof(addr));
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_channel = channel;
	
	if (!stringTobaddr(address, addr.rc_bdaddr))
	{
		spdlog::error("[Client] 无效的蓝牙地址: {}", address);
		close(_socket);
		_socket = -1;
		return false;
	}

	// 尝试连接
	int result = ::connect(_socket, (struct sockaddr*)&addr, sizeof(addr));
	if (result < 0)
	{
		if (errno == EINPROGRESS)
		{
			// 非阻塞连接，等待连接完成
			fd_set writefds, errorfds;
			FD_ZERO(&writefds);
			FD_ZERO(&errorfds);
			FD_SET(_socket, &writefds);
			FD_SET(_socket, &errorfds);
			
			struct timeval timeout;
			timeout.tv_sec = 5; // 5秒超时
			timeout.tv_usec = 0;
			
			result = select(_socket + 1, nullptr, &writefds, &errorfds, &timeout);
			
			if (result > 0)
			{
				if (FD_ISSET(_socket, &writefds))
				{
					// 检查连接是否成功
					int error = 0;
					socklen_t len = sizeof(error);
					if (getsockopt(_socket, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0)
					{
						spdlog::error("[Client] 连接失败: {}", strerror(error));
						close(_socket);
						_socket = -1;
						return false;
					}
				}
				else if (FD_ISSET(_socket, &errorfds))
				{
					spdlog::error("[Client] 连接出错");
					close(_socket);
					_socket = -1;
					return false;
				}
			}
			else
			{
				spdlog::error("[Client] 连接超时");
				close(_socket);
				_socket = -1;
				return false;
			}
		}
		else
		{
			spdlog::error("[Client] 连接失败: {}", strerror(errno));
			close(_socket);
			_socket = -1;
			return false;
		}
	}

	// 恢复阻塞模式
	flags = fcntl(_socket, F_GETFL, 0);
	if (flags >= 0)
	{
		fcntl(_socket, F_SETFL, flags & ~O_NONBLOCK);
	}

	return true;
}

void BluetoothClient::cleanupConnection()
{
	std::lock_guard<std::mutex> lock(_socketMutex);
	
	if (_socket >= 0)
	{
		close(_socket);
		_socket = -1;
	}
}

std::string BluetoothClient::bdaddrToString(const sockaddr_rc& addr)
{
	char str[20];
	ba2str(&addr.rc_bdaddr, str);
	return std::string(str);
}

bool BluetoothClient::stringTobaddr(const std::string& str, bdaddr_t& addr)
{
	return str2ba(str.c_str(), &addr) == 0;
}

void BluetoothClient::handleError(const std::string& message)
{
	spdlog::error("[Client] 错误: {}", message);
	
	if (_errorCallback)
	{
		std::lock_guard<std::mutex> lock(_callbackMutex);
		_errorCallback(message);
	}
}