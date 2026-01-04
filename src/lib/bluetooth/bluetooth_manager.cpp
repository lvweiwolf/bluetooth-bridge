#include <bluetooth/bluetooth_manager.h>
#include <bluetooth/utils.h>
#include <utils/logger.h>

#include <sstream>

BluetoothManager::BluetoothManager(sdbus::IConnection& conn_adpater,
								   sdbus::IConnection& conn_devices)
	: ProxyInterfaces(conn_adpater, sdbus::ServiceName(INTERFACE_NAME), sdbus::ObjectPath("/")),
	  _max_repair_count(3),
	  _max_reconnect_count(3),
	  _timeout_pair_ms(1000),
	  _timeout_connect_ms(1000),
	  _conn_devices(conn_devices)
{
	registerProxy();

	for (const auto& [object, interfaceAndProperties] : GetManagedObjects())
	{
		onInterfacesAdded(object, interfaceAndProperties);
	}
}

BluetoothManager::~BluetoothManager() { unregisterProxy(); }

Json::Value BluetoothManager::getAdapters()
{
	// 获得所有蓝牙适配器
	Json::Value adaptersValue(Json::arrayValue);

	for (const auto& [objectPath, adapterPtr] : _adapters)
	{
		if (adapterPtr)
		{
			Json::Value adapterValue = adapterPtr->getProperties().toJson();
			adaptersValue.append(adapterValue);
		}
	}

	Json::Value root;
	root["adapters"] = adaptersValue;
	return root;
}

Json::Value BluetoothManager::getDevices()
{
	Json::Value root;
	Json::Value devicesValue(Json::arrayValue);

	for (const auto& [objectPath, device] : _devices)
	{
		Json::Value deviceValue = device->getProperties().toJson();
		devicesValue.append(deviceValue);
	}

	root["devices"] = devicesValue;
	return root;
}

bool BluetoothManager::getPincode(const std::string& devicePath,
								  std::string& pincode,
								  bool removeIt)
{
	std::lock_guard<std::mutex> lock(_pincodes_mutex);

	auto it = _pairing_pincodes.find(devicePath);

	if (it != _pairing_pincodes.end())
	{
		pincode = it->second;

		if (removeIt)
			_pairing_pincodes.erase(it);

		return true;
	}

	return false;
}

bool BluetoothManager::requestConnect(const std::string& address, std::string& err)
{
	// 蓝牙适配器
	if (_adapters.empty())
	{
		err = "未找到蓝牙适配器";
		return false;
	}

	// 设备对象路径
	std::unique_ptr<Device> device;
	auto deviceObjectPath = getDevicePath(address);

	// 查找设备对象
	for (const auto& [objectPath, interfaceAndProperties] : GetManagedObjects())
	{
		if (objectPath != deviceObjectPath)
			continue;

		for (const auto& [interface, properties] : interfaceAndProperties)
		{
			if (interface != org::bluez::Device1_proxy::INTERFACE_NAME)
				continue;

			auto deviceFound = std::make_unique<Device>(_conn_devices,
														sdbus::ServiceName(INTERFACE_NAME),
														objectPath,
														properties);

			device = std::move(deviceFound);
			break;
		}

		if (device)
			break;
	}

	// 查找发现设备
	if (!device)
	{
		err = "设备未发现，尝试发现设备...";
		return false;
	}

	int numRepeat = 0;

	// 设备配对
	{
		bool pairRepeat = true;
		std::chrono::milliseconds ms(_timeout_pair_ms);
		auto timeout = std::chrono::duration_cast<std::chrono::microseconds>(ms);
		auto startTime = std::chrono::steady_clock::now();

		while (!device->paired())
		{
			if (numRepeat >= _max_repair_count)
				break;

			auto nowTime = std::chrono::steady_clock::now();
			auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - startTime);

			// 配对超时，中断
			if (cost.count() >= _timeout_pair_ms)
			{
				LOG_ERROR("配对超时");
				break;
			}

			if (pairRepeat)
			{
				try
				{
					device->pair(timeout.count());
					pairRepeat = false;
				}
				catch (const sdbus::Error& e)
				{
					sdbus::Error::Name errName = e.getName();
					if (errName != "org.bluez.Error.AlreadyExists")
					{
						LOG_DEBUG("请求配对设备失败 - {}/{}", e.getName(), e.getMessage());
						LOG_WARN("配对异常，重试配对({}/{})...", numRepeat, _max_repair_count);
						++numRepeat;
					}
					
					pairRepeat = true;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		};
	}

	numRepeat = 0;

	// 设备连接
	{
		bool connectRepeat = true;
		std::chrono::milliseconds ms(_timeout_connect_ms);
		auto timeout = std::chrono::duration_cast<std::chrono::microseconds>(ms);
		auto startTime = std::chrono::steady_clock::now();

		while (!device->connected())
		{
			if (numRepeat >= _max_reconnect_count)
				break;

			auto nowTime = std::chrono::steady_clock::now();
			auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - startTime);

			// 连接超时，中断
			if (cost.count() >= _timeout_connect_ms)
			{
				LOG_ERROR("连接超时");
				break;
			}

			if (connectRepeat)
			{
				try
				{
					device->connect(timeout.count());
					connectRepeat = false;
				}
				catch (const sdbus::Error& e)
				{
					sdbus::Error::Name errName = e.getName();

					// 连接失败，重试连接
					if (errName == "org.bluez.Error.Failed" ||
						errName == "org.bluez.Error.NotReady" ||
						errName == "org.bluez.Error.BREDR.ProfileUnavailable")
					{
						LOG_DEBUG("请求连接设备失败 - {}/{}", e.getName(), e.getMessage());
						LOG_WARN("连接异常，重试连接({}/{})...", numRepeat, _max_reconnect_count);
						++numRepeat;
					}
					
					connectRepeat = true;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	bool paired = device->paired();
	bool connected = device->connected();

	if (!paired)
		err = "设备配对失败, 设备: " + address;

	if (!connected)
		err = "设备连接失败, 设备: " + address;

	return paired && connected;
}

bool BluetoothManager::requestConnectWithPincode(const std::string& address,
												 const std::string& pincode,
												 std::string& err)
{

	auto objectPath = getDevicePath(address);

	{
		std::lock_guard<std::mutex> lock(_pincodes_mutex);
		_pairing_pincodes[objectPath] = pincode;
	}

	return requestConnect(address, err);
}

bool BluetoothManager::requestRemoveDevice(const std::string& address, std::string& err)
{
	auto objectPath = getDevicePath(address);

	try
	{
		// 1. 在已发现设备上查找
		auto& adpater = _adapters.begin()->second;
		adpater->removeDevice(objectPath);
	}
	catch (const sdbus::Error& e)
	{
		std::string errName = e.getName();

		if (errName == "org.bluez.Error.Failed")
		{
			err = "移除设备失败";
			LOG_ERROR("移除设备失败 ({}/{})...", e.getName(), e.getMessage());
			return false;
		}
		else
			LOG_WARN("移除设备异常 ({}/{})...", e.getName(), e.getMessage());
	}

	return true;
}

const Device* BluetoothManager::findDevice(const std::string& address)
{
	// 查找设备对象
	auto objectPath = getDevicePath(address);

	std::scoped_lock lock(_devices_mutex);
	auto it = _devices.find(objectPath);
	if (it != _devices.end())
		return it->second.get();

	return nullptr;
}

void BluetoothManager::onInterfacesAdded(
	const sdbus::ObjectPath& objectPath,
	const std::map<sdbus::InterfaceName, std::map<sdbus::PropertyName, sdbus::Variant>>&
		interfaceAndProperties)
{
	std::ostringstream os;
	os << std::endl;

	for (const auto& [interface, properties] : interfaceAndProperties)
	{
		if (interface == PROPERTIES_INTERFACE_NAME || interface == INTROSPECTABLE_INTERFACE_NAME)
			continue;

		os << "[" << objectPath << "] Add - " << interface << std::endl;

		if (!properties.empty())
		{
			Utils::appendProperties(properties, os);
		}

		if (interface == org::bluez::Adapter1_proxy::INTERFACE_NAME)
		{
			std::scoped_lock lock(_adapters_mutex);
			if (!_adapters.count(objectPath))
			{
				auto adapter1 = std::make_unique<Adapter>(getProxy().getConnection(),
														  sdbus::ServiceName(INTERFACE_NAME),
														  objectPath,
														  properties);
				_adapters[objectPath] = std::move(adapter1);
			}
		}
		else if (interface == org::bluez::Device1_proxy::INTERFACE_NAME)
		{
			std::scoped_lock lock(_devices_mutex);
			if (!_devices.count(objectPath))
			{
				auto device = std::make_unique<Device>(getProxy().getConnection(),
													   sdbus::ServiceName(INTERFACE_NAME),
													   objectPath,
													   properties);

				_devices[objectPath] = std::move(device);
			}
		}
	}

	LOG_DEBUG(os.str());
}


void BluetoothManager::onInterfacesRemoved(const sdbus::ObjectPath& objectPath,
										   const std::vector<sdbus::InterfaceName>& interfaceNames)
{
	std::ostringstream os;
	os << std::endl;

	for (const auto& interface : interfaceNames)
	{
		if (interface == PROPERTIES_INTERFACE_NAME || interface == INTROSPECTABLE_INTERFACE_NAME)
			continue;

		os << "[" << objectPath << "] Remove - " << interface << std::endl;

		if (interface == org::bluez::Adapter1_proxy::INTERFACE_NAME)
		{
			std::scoped_lock lock(_adapters_mutex);
			if (_adapters.count(objectPath))
			{
				_adapters[objectPath].reset();
				_adapters.erase(objectPath);
			}
		}
		else if (interface == org::bluez::Device1_proxy::INTERFACE_NAME)
		{
			std::scoped_lock lock(_devices_mutex);
			if (_devices.count(objectPath))
			{
				_devices[objectPath].reset();
				_devices.erase(objectPath);
			}
		}
	}

	LOG_DEBUG(os.str());
}

sdbus::ObjectPath BluetoothManager::getDevicePath(const std::string& address) const
{
	// 查找设备对象
	std::string devicePath = address;
	std::replace(devicePath.begin(), devicePath.end(), ':', '_');

	sdbus::ObjectPath deviceObjectPath =
		sdbus::ObjectPath(_adapters.begin()->first + "/dev_" + devicePath);

	return deviceObjectPath;
}