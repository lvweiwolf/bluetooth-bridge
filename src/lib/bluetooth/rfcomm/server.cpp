#include <bluetooth/rfcomm/server.h>
#include <bluetooth/rfcomm/sdp.h>

#include <sstream>

#include <cctype>
#include <unistd.h>
#include <fcntl.h>

// SPP UUID定义 (00001101-0000-1000-8000-00805F9B34FB)
const char* BluetoothServer::SPP_UUID = "00001101-0000-1000-8000-00805F9B34FB";
// const uint8_t BluetoothServer::SPP_UUID[16] = { 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00,
// 												0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

BluetoothServer::BluetoothServer(const std::string& name, uint8_t channel)
	: _serverName(name),
	  _channel(channel),
	  _srvSocket(-1),
	  _running(false),
	  _nextClientId(1),
	  _clientConnectCallback(nullptr),
	  _clientDisconnectCallback(nullptr),
	  _dataReceivedCallback(nullptr),
	  _errorCallback(nullptr),
	  _sdp_handle(0)
{
	// 设置默认回调
	_clientConnectCallback = [](int id, const std::string& addr) {
		spdlog::info("[Server] 客户端 {} ({}) 连接到服务器", id, addr);
	};

	_clientDisconnectCallback = [](int id, const std::string& addr) {
		spdlog::info("[Server] 客户端 {} ({}) 连接断开", id, addr);
	};

	_dataReceivedCallback = [](int id, const std::string& data) {
		spdlog::info("[Server] 从客户端 {} 获取数据: {}", id,
					data.length() > 50 ? data.substr(0, 50) + "..." : data);
	};

	_errorCallback = [](int id, const std::string& error) {
		spdlog::error("[Server] 客户端 {} 发生错误: {}", id, error);
	};

	if (_channel == 0)
	{
		// 查找RFCOMM通道
		uint8_t rfcommChannel = 0;
		if (sdp::findAvailableSPPChannel("local", rfcommChannel) && rfcommChannel != 0)
		{
			_channel = rfcommChannel;
		}
		else
		{
			_channel = 1; // 默认通道
			sdp::registerSPPService(_serverName, SPP_UUID, _channel, _sdp_handle);
		}
	}
	else
	{
		sdp::registerSPPService(_serverName, SPP_UUID, _channel, _sdp_handle);
	}
}

BluetoothServer::~BluetoothServer()
{
	if (_sdp_handle)
		sdp::unregisterSPPService(_sdp_handle);

	stop();
}

bool BluetoothServer::start()
{
	if (_running)
	{
		_errorCallback(-1, "服务器已经在运行");
		return false;
	}

	// 创建套接字
	_srvSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if (_srvSocket < 0)
	{
		_errorCallback(-1, "创建套接字失败: " + std::string(strerror(errno)));
		close(_srvSocket);
		_srvSocket = -1;
		return false;
	}

	// 设置套接字选项(重用地址)
	int reuse = 1;
	if (setsockopt(_srvSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{
		_errorCallback(-1, "设置套接字选项失败: " + std::string(strerror(errno)));
		close(_srvSocket);
		_srvSocket = -1;
		return false;
	}

	// 绑定地址
	sockaddr_rc addr;
	addr.rc_family = AF_BLUETOOTH;
	addr.rc_bdaddr = { { 0, 0, 0, 0, 0, 0 } };
	addr.rc_channel = _channel;

	if (bind(_srvSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		_errorCallback(-1, "绑定套接字失败: " + std::string(strerror(errno)));
		close(_srvSocket);
		_srvSocket = -1;
		return false;
	}

	// 开始监听
	if (listen(_srvSocket, 1) < 0)
	{
		_errorCallback(-1, "监听套接字失败: " + std::string(strerror(errno)));
		close(_srvSocket);
		_srvSocket = -1;
		return false;
	}

	// 设置非阻塞模式，以便在停止服务器时能及时退出
	int flags = fcntl(_srvSocket, F_GETFL, 0);
	fcntl(_srvSocket, F_SETFL, flags | O_NONBLOCK);

	_running = true;

	// 启动接受连接线程
	_acceptThread = std::thread(&BluetoothServer::acceptThread, this);

	spdlog::info("[Server] {} 启动 RFCOMM, channel: {}", _serverName,
			 static_cast<int>(_channel));
	spdlog::info("[Server] 本地地址: {}", getLocalAddress());

	return true;
}

void BluetoothServer::stop()
{
	if (!_running)
		return;

	_running = false;

	// 关闭服务器套接字以中断阻塞的accept调用
	if (_srvSocket >= 0)
	{
		close(_srvSocket);
		_srvSocket = -1;
	}

	// 等待接受线程结束
	if (_acceptThread.joinable())
		_acceptThread.join();

	// 断开所有客户端连接
	std::vector<int> clientIds;
	{
		std::lock_guard<std::mutex> lock(_clientsMutex);
		for (const auto& [id, client] : _clients)
			clientIds.push_back(id);
	}

	for (int id : clientIds)
		disconnectClient(id);

	spdlog::info("[Server] {} 停止 RFCOMM", _serverName);
}

void BluetoothServer::acceptThread()
{
	while (_running)
	{
		struct sockaddr_rc clientAddr;
		socklen_t len = sizeof(clientAddr);

		// 使用select实现带超时的accept
		fd_set readFds;
		FD_ZERO(&readFds);
		FD_SET(_srvSocket, &readFds);

		struct timeval tv = { 1, 0 }; // 1秒超时
		int ret = select(_srvSocket + 1, &readFds, nullptr, nullptr, &tv);

		if (ret < 0)
		{
			if (errno != EINTR)
				_errorCallback(-1, "Select 错误: " + std::string(strerror(errno)));

			continue;
		}

		// 超时，继续循环
		if (ret == 0)
			continue;

		if (FD_ISSET(_srvSocket, &readFds))
		{
			int clientSocket = accept(_srvSocket, (struct sockaddr*)&clientAddr, &len);

			if (clientSocket < 0)
			{
				if (errno != EAGAIN && errno != EWOULDBLOCK)
					_errorCallback(-1, "接受连接错误: " + std::string(strerror(errno)));

				continue;
			}

			// 获取客户端信息
			int clientId = getNextClientId();
			auto clientInfo = std::make_shared<ClientInfo>();
			clientInfo->socket = clientSocket;
			clientInfo->address = bdaddrToString(clientAddr);
			clientInfo->running = true;
			clientInfo->connectTime = std::chrono::steady_clock::now();

			// 保存客户端信息
			{
				std::lock_guard<std::mutex> lock(_clientsMutex);
				_clients[clientId] = clientInfo;
			}

			// 启动客户端线程
			std::shared_ptr<ClientInfo> info = _clients[clientId];
			info->workThread = std::thread(&BluetoothServer::clientThread, this, clientId, info);

			// 调用连接回调
			_clientConnectCallback(clientId, info->address);

			// 发送欢迎消息
			std::string welcome =
				"Welcome to " + _serverName + " (Client ID: " + std::to_string(clientId) + ")\n";

			send(clientSocket, welcome.c_str(), welcome.length(), 0);
		}
	}
}

void BluetoothServer::clientThread(int clientId, std::shared_ptr<ClientInfo> clientInfo)
{
	int clientSocket = clientInfo->socket;
	std::string clientAddr = clientInfo->address;

	// 设置接受超时
	struct timeval tv = { 1, 0 }; // 1秒超时
	setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	char buffer[4096];

	while (clientInfo->running && _running)
	{
		memset(buffer, 0, sizeof(buffer));
		ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

		if (bytesReceived > 0)
		{
			buffer[bytesReceived] = '\0';
			std::string data(buffer);

			// 调用数据接受回调
			_dataReceivedCallback(clientId, data);
		}
		else if (bytesReceived == 0)
		{
			// 客户端断开连接
			break;
		}
		else
		{
			if (errno != EWOULDBLOCK && errno != EAGAIN)
			{
				_errorCallback(clientId, "接受数据错误: " + std::string(strerror(errno)));
				break;
			}

			// 超时继续循环
		}
	}

	// 清理客户端资源
	cleanupClient(clientId);

	// 调用断开连接回调
	_clientDisconnectCallback(clientId, clientAddr);
}

ssize_t BluetoothServer::sendToClient(int clientId, const std::string& data)
{
	std::lock_guard<std::mutex> lock(_clientsMutex);

	auto it = _clients.find(clientId);
	if (it == _clients.end() || !it->second->running)
	{
		_errorCallback(clientId, "发送数据失败: 客户端未连接");
		return -1;
	}

	ssize_t bytesSent = send(it->second->socket, data.c_str(), data.length(), 0);

	if (bytesSent < 0)
		_errorCallback(clientId, "发送数据错误: " + std::string(strerror(errno)));

	return bytesSent;
}

size_t BluetoothServer::broadcast(const std::string& data)
{
	std::lock_guard<std::mutex> lock(_clientsMutex);
	size_t numSuccess = 0;

	for (auto& [clientId, clientInfo] : _clients)
	{
		if (clientInfo->running)
		{
			ssize_t bytesSent = send(clientInfo->socket, data.c_str(), data.length(), 0);

			if (bytesSent > 0)
				++numSuccess;
		}
	}

	return numSuccess;
}

void BluetoothServer::disconnectClient(int clientId)
{
	std::shared_ptr<ClientInfo> clientInfo;

	{
		std::lock_guard<std::mutex> lock(_clientsMutex);
		auto it = _clients.find(clientId);

		if (it == _clients.end())
			return;

		clientInfo = std::move(it->second);
		_clients.erase(it);
	}

	if (clientInfo)
	{
		clientInfo->running = false;

		// 关闭套接字以中断recv调用
		if (clientInfo->socket >= 0)
			close(clientInfo->socket);

		// 等待线程结束
		if (clientInfo->workThread.joinable())
			clientInfo->workThread.join();
	}
}
std::vector<int> BluetoothServer::getConnectedClients() const
{
	std::vector<int> result;
	std::lock_guard<std::mutex> lock(_clientsMutex);

	for (const auto& [id, client] : _clients)
	{
		if (client->running)
			result.push_back(id);
	}

	return result;
}

std::string BluetoothServer::getClientInfo(int clientId) const
{
	std::lock_guard<std::mutex> lock(_clientsMutex);

	auto it = _clients.find(clientId);
	if (it == _clients.end())
		return "客户端未连接";

	const auto& clientInfo = it->second;
	auto now = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - clientInfo->connectTime);

	std::stringstream ss;
	ss << "Client ID: " << clientId << "\n"
	   << "Address: " << clientInfo->address << "\n"
	   << "Connected for: " << duration.count() << " seconds\n"
	   << "Status: " << (clientInfo->running ? "Connected" : "Disconnecting");

	return ss.str();
}

std::string BluetoothServer::getLocalAddress() const
{
	char addrBuf[19] = { 0 };
	bdaddr_t bdaddr;

	if (hci_devba(0, &bdaddr) < 0)
	{
		return "Unknown";
	}

	ba2str(&bdaddr, addrBuf);
	return std::string(addrBuf);
}


void BluetoothServer::cleanupClient(int clientId)
{
	std::lock_guard<std::mutex> lock(_clientsMutex);
	_clients.erase(clientId);
}

int BluetoothServer::getNextClientId() { return _nextClientId++; }

std::string BluetoothServer::bdaddrToString(const sockaddr_rc& addr)
{
	char addr_str[19] = { 0 };
	ba2str(&addr.rc_bdaddr, addr_str);
	return std::string(addr_str);
}

bool BluetoothServer::stringTobaddr(const std::string& str, bdaddr_t& bdaddr)
{
	return str2ba(str.c_str(), &bdaddr) == 0;
}