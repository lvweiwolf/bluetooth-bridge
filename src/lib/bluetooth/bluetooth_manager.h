#ifndef BLUETOOTH_CLIENT_H_
#define BLUETOOTH_CLIENT_H_

#include <defines.h>
#include <bluetooth/adapter.h>
#include <bluetooth/device.h>

#include <json/json.h>

class CORE_API BluetoothManager : public sdbus::ProxyInterfaces<sdbus::ObjectManager_proxy>
{
public:
	BluetoothManager(sdbus::IConnection& conn_adapter, sdbus::IConnection& conn_devices);

	~BluetoothManager();

	Json::Value getAdapters();

	Json::Value getDevices();

	bool getPincode(const std::string& devicePath, std::string& pincode, bool removeIt = true);

	bool requestConnect(const std::string& address, std::string& err);

	bool requestConnectWithPincode(const std::string& address,
								   const std::string& pincode,
								   std::string& err);

	bool requestRemoveDevice(const std::string& address, std::string& err);

	const Device* findDevice(const std::string& address);

	void setMaxRepairCount(int maxRepairCount) { _max_repair_count = std::max(1, maxRepairCount); }

	void setMaxReconnectCount(int maxReconnectCount)
	{
		_max_reconnect_count = std::max(1, maxReconnectCount);
	}

	void setPairTimeout(int timeoutMs) { _timeout_pair_ms = std::max(0, timeoutMs); }

	void setConnectTimeout(int timeoutMs) { _timeout_connect_ms = std::max(0, timeoutMs); }

private:
	void onInterfacesAdded(
		const sdbus::ObjectPath& objectPath,
		const std::map<sdbus::InterfaceName, std::map<sdbus::PropertyName, sdbus::Variant>>&
			interfaceAndProperties) override;


	void onInterfacesRemoved(const sdbus::ObjectPath& objectPath,
							 const std::vector<sdbus::InterfaceName>& interfaces) override;

	sdbus::ObjectPath getDevicePath(const std::string& address) const;

private:
	static constexpr auto INTERFACE_NAME = "org.bluez";
	static constexpr auto PROPERTIES_INTERFACE_NAME = "org.freedesktop.DBus.Properties";
	static constexpr auto INTROSPECTABLE_INTERFACE_NAME = "org.freedesktop.DBus.Introspectable";


	int _max_repair_count;
	int _max_reconnect_count;
	int _timeout_pair_ms;
	int _timeout_connect_ms;

	std::mutex _adapters_mutex;
	std::mutex _devices_mutex;

	std::map<sdbus::ObjectPath, std::unique_ptr<Adapter>> _adapters;
	std::map<sdbus::ObjectPath, std::unique_ptr<Device>> _devices;

	std::mutex _pincodes_mutex;
	std::map<std::string, std::string> _pairing_pincodes;

	sdbus::IConnection& _conn_devices;
};



#endif // BLUETOOTH_CLIENT_H_
