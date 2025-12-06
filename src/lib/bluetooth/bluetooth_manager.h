#ifndef BLUETOOTH_CLIENT_H_
#define BLUETOOTH_CLIENT_H_

#include <defines.h>
#include <bluetooth/adapter.h>
#include <bluetooth/device.h>

#include <json/json.h>

class CORE_API BluetoothManager : public sdbus::ProxyInterfaces<sdbus::ObjectManager_proxy>
{
public:
	BluetoothManager(sdbus::IConnection& connection);

	~BluetoothManager();

	Json::Value getPairedDevices();

private:
	void onInterfacesAdded(
		const sdbus::ObjectPath& objectPath,
		const std::map<sdbus::InterfaceName, std::map<sdbus::PropertyName, sdbus::Variant>>&
			interfaceAndProperties) override;


	void onInterfacesRemoved(const sdbus::ObjectPath& objectPath,
							 const std::vector<sdbus::InterfaceName>& interfaces) override;

private:
	static constexpr auto INTERFACE_NAME = "org.bluez";
	static constexpr auto PROPERTIES_INTERFACE_NAME = "org.freedesktop.DBus.Properties";
	static constexpr auto INTROSPECTABLE_INTERFACE_NAME = "org.freedesktop.DBus.Introspectable";


	std::map<sdbus::ObjectPath, std::unique_ptr<Adapter>> _adapters;
	std::map<sdbus::ObjectPath, std::unique_ptr<Device>> _devices;
	std::map<sdbus::ObjectPath, Device::Properties> _paired_devices;
};



#endif // BLUETOOTH_CLIENT_H_