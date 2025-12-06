#ifndef BLUETOOTH_ADAPTER_H
#define BLUETOOTH_ADAPTER_H

#include <bluetooth/proxy/adapter_proxy.h>
#include <map>

class Adapter : public sdbus::ProxyInterfaces<sdbus::Properties_proxy, org::bluez::Adapter1_proxy>
{
public:
	struct Properties
	{
		bool discoverable;
		bool discovering;
		bool pairable;
		bool powered;
		std::string address;
		std::string addressType;
		std::string alias;
		std::string modalias;
		std::string name;
		std::uint32_t classType;
		std::uint32_t discoverableTimeout;
		std::uint32_t pairableTimeout;
		std::vector<std::string> uuids;
	};

	Adapter(sdbus::IConnection& connection,
			 const sdbus::ServiceName& destination,
			 const sdbus::ObjectPath& objectPath,
			 const std::map<sdbus::PropertyName, sdbus::Variant>& properties)
		: ProxyInterfaces(connection, destination, objectPath)
	{
		registerProxy();
		onPropertiesChanged(sdbus::InterfaceName(Adapter1_proxy::INTERFACE_NAME), properties, {});
	}

	virtual ~Adapter() { unregisterProxy(); }

	[[nodiscard]] const Properties getProperties() const { return _properties; }


private:
	void onPropertiesChanged(const sdbus::InterfaceName& interfaceName,
							 const std::map<sdbus::PropertyName, sdbus::Variant>& changedProperties,
							 const std::vector<sdbus::PropertyName>& invalidatedProperties) override
	{
		if (const auto key = sdbus::MemberName("Address"); changedProperties.count(key))
		{
			_properties.address = changedProperties.at(key).get<std::string>();
		}
		if (const auto key = sdbus::MemberName("AddressType"); changedProperties.count(key))
		{
			_properties.addressType = changedProperties.at(key).get<std::string>();
		}
		if (const auto key = sdbus::MemberName("Alias"); changedProperties.count(key))
		{
			_properties.alias = changedProperties.at(key).get<std::string>();
		}
		if (const auto key = sdbus::MemberName("Class"); changedProperties.count(key))
		{
			_properties.classType = changedProperties.at(key).get<std::uint32_t>();
		}
		if (const auto key = sdbus::MemberName("Discoverable"); changedProperties.count(key))
		{
			_properties.discoverable = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("DiscoverableTimeout");
			changedProperties.count(key))
		{
			_properties.discoverableTimeout = changedProperties.at(key).get<std::uint32_t>();
		}
		if (const auto key = sdbus::MemberName("Discovering"); changedProperties.count(key))
		{
			_properties.discovering = changedProperties.at(key).get<bool>();

			if (!_properties.discovering)
			{
				this->startDiscovery();
			}
		}
		if (const auto key = sdbus::MemberName("Modalias"); changedProperties.count(key))
		{
			_properties.modalias = changedProperties.at(key).get<std::string>();
		}
		if (const auto key = sdbus::MemberName("Name"); changedProperties.count(key))
		{
			_properties.name = changedProperties.at(key).get<std::string>();
		}
		if (const auto key = sdbus::MemberName("Pairable"); changedProperties.count(key))
		{
			_properties.pairable = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("PairableTimeout"); changedProperties.count(key))
		{
			_properties.pairableTimeout = changedProperties.at(key).get<std::uint32_t>();
		}
		if (const auto key = sdbus::MemberName("Powered"); changedProperties.count(key))
		{
			_properties.powered = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("UUIDs"); changedProperties.count(key))
		{
			_properties.uuids = changedProperties.at(key).get<std::vector<std::string>>();
		}
	}

private:
	Properties _properties {};
	
};

#endif