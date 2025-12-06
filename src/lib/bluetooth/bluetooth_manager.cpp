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