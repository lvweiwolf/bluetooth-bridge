#include <bluetooth/bluetooth_manager.h>
#include <bluetooth/utils.h>
#include <sstream>

BluetoothManager::BluetoothManager(sdbus::IConnection& connection)
	: ProxyInterfaces(connection, sdbus::ServiceName(INTERFACE_NAME), sdbus::ObjectPath("/"))
{
	registerProxy();

	for (const auto& [object, interfaceAndProperties] : GetManagedObjects())
	{
		onInterfacesAdded(object, interfaceAndProperties);
	}
}

BluetoothManager::~BluetoothManager() { unregisterProxy(); }

Json::Value BluetoothManager::getPairedDevices()
{
	// 获得当前发现的设备
	std::vector<std::unique_ptr<Device>> devices;

	for (const auto& [objectPath, interfaceAndProperties] : GetManagedObjects())
	{
		for (const auto& [interface, properties] : interfaceAndProperties)
		{
			if (interface != org::bluez::Device1_proxy::INTERFACE_NAME)
				continue;

			auto device = std::make_unique<Device>(getProxy().getConnection(),
												   sdbus::ServiceName(INTERFACE_NAME),
												   objectPath,
												   properties);

			devices.emplace_back(std::move(device));
		}
	}

	Json::Value root;
	Json::Value devicesValue(Json::arrayValue);

	for (const auto& devicePtr : devices)
	{
		Json::Value deviceValue = devicePtr->getProperties().toJson();;
		devicesValue.append(deviceValue);
	}

	root["devices"] = devicesValue;
	return root;
}

bool BluetoothManager::requestConnect(const std::string& deviceAddress)
{
	// 蓝牙适配器
	if (_adapters.empty())
	{
		spdlog::error("未找到蓝牙适配器");
		return false;
	}

	// 查找设备对象
	std::string devicePath = deviceAddress;
	std::replace(devicePath.begin(), devicePath.end(), ':', '_');

	sdbus::ObjectPath deviceObjectPath =
		sdbus::ObjectPath(_adapters.begin()->first + "/dev_" + devicePath);

	// 查找发现设备
	auto it = _devices.find(deviceObjectPath);
	if (it != _devices.end())
	{
		// 设备已连接，直接返回
		auto& device = it->second;
		if (device->getProperties().connected)
			return true;
	}

	// 尝试发现设备
	try
	{
		// 1. 在已发现设备上查找
		auto& adpater = _adapters.begin()->second;
		adpater->removeDevice(deviceObjectPath);
		
		std::map<std::string, sdbus::Variant> connectionProps = { { "Address",
																	sdbus::Variant(deviceAddress) } };
		adpater->connectDevice(connectionProps);
	}
	catch (const sdbus::Error& e)
	{
		spdlog::error("请求连接设备失败: {}", e.getMessage());
		return false;
	}

	return true;
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

	spdlog::debug(os.str());
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
			if (_adapters.count(objectPath))
			{
				_adapters[objectPath].reset();
				_adapters.erase(objectPath);
			}
		}
		else if (interface == org::bluez::Device1_proxy::INTERFACE_NAME)
		{
			if (_devices.count(objectPath))
			{
				_devices[objectPath].reset();
				_devices.erase(objectPath);
			}
		}
	}

	spdlog::debug(os.str());
}