#ifndef BLUETOOTH_DEVICE_H_
#define BLUETOOTH_DEVICE_H_

#include <defines.h>
#include <bluetooth/proxy/device_proxy.h>
#include <spdlog/spdlog.h>
#include <json/json.h>

#include <regex>
#include <optional>

class CORE_API Device final
	: public sdbus::ProxyInterfaces<sdbus::Properties_proxy, org::bluez::Device1_proxy>
{
public:
	struct Modalias
	{
		std::string vid;
		std::string pid;
		std::string did;
	};

	struct Properties
	{
		std::string name;
		std::string adapter;
		std::string address;
		std::string address_type;
		std::string alias;
		std::optional<Modalias> modalias;
		std::vector<std::string> uuids;
		std::map<std::uint16_t, sdbus::Variant> manufacturerData;
		std::map<std::string, sdbus::Variant> serviceData;
		std::int16_t rssi;
		bool blocked;
		bool bonded;
		bool connected;
		bool legacyPairing;
		bool paired;
		bool servicesResolved;
		bool trusted;

		Json::Value toJson() const
		{
			Json::Value root;
			root["name"] = name;
			root["adapter"] = adapter;
			root["address"] = address;
			root["address_type"] = address_type;
			root["alias"] = alias;
			root["rssi"] = rssi;
			root["blocked"] = blocked;
			root["bonded"] = bonded;
			root["connected"] = connected;
			root["legacyPairing"] = legacyPairing;
			root["paired"] = paired;
			root["servicesResolved"] = servicesResolved;
			root["trusted"] = trusted;

			if (modalias)
			{
				Json::Value mod_alias;
				mod_alias["vid"] = modalias->vid;
				mod_alias["pid"] = modalias->pid;
				mod_alias["did"] = modalias->did;
				root["modalias"] = mod_alias;
			}

			Json::Value uuids_json(Json::arrayValue);
			for (const auto& uuid : uuids)
			{
				uuids_json.append(uuid);
			}
			root["uuids"] = uuids_json;

			return root;
		}
	};

	Device(sdbus::IConnection& connection,
		   const sdbus::ServiceName(&destination),
		   const sdbus::ObjectPath(&objectPath),
		   const std::map<sdbus::PropertyName, sdbus::Variant>& properties)
		: ProxyInterfaces{ connection, destination, objectPath }
	{
		registerProxy();
		spdlog::debug("Device1: {}", objectPath);
		onPropertiesChanged(sdbus::InterfaceName(Device1_proxy::INTERFACE_NAME), properties, {});
	}
	virtual ~Device() { unregisterProxy(); }

	[[nodiscard]] const Properties& getProperties() const { return _properties; }


	static std::optional<Modalias> parseModalias(const std::string& mod_alias)
	{
		const std::regex re(R"(usb:v([0-9A-Fa-f]{4})p([0-9A-Fa-f]{4})d([0-9A-Fa-f]{4}))");

		if (std::smatch match; std::regex_search(mod_alias, match, re) && match.size() == 4)
		{
			return Modalias{ match[1].str(), match[2].str(), match[3].str() };
		}

		const std::regex re2(R"(bluetooth:v([0-9A-Fa-f]{4})p([0-9A-Fa-f]{4})d([0-9A-Fa-f]{4}))");

		if (std::smatch match; std::regex_search(mod_alias, match, re2) && match.size() == 4)
		{
			return Modalias{ match[1].str(), match[2].str(), match[3].str() };
		}

		spdlog::error("解析模态失败: {}", mod_alias);
		return {};
	}

private:
	Properties _properties{};

	void onPropertiesChanged(const sdbus::InterfaceName& interfaceName,
							 const std::map<sdbus::PropertyName, sdbus::Variant>& changedProperties,
							 const std::vector<sdbus::PropertyName>& invalidatedProperties) override
	{
		if (const auto key = sdbus::MemberName("Adapter"); changedProperties.count(key))
		{
			_properties.adapter = changedProperties.at(key).get<sdbus::ObjectPath>();
		}
		if (const auto key = sdbus::MemberName("Address"); changedProperties.count(key))
		{
			_properties.address = changedProperties.at(key).get<std::string>();
		}
		if (const auto key = sdbus::MemberName("AddressType"); changedProperties.count(key))
		{
			_properties.address_type = changedProperties.at(key).get<std::string>();
		}
		if (const auto key = sdbus::MemberName("Bonded"); changedProperties.count(key))
		{
			_properties.bonded = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("Blocked"); changedProperties.count(key))
		{
			_properties.blocked = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("Connected"); changedProperties.count(key))
		{
			_properties.connected = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("LegacyPairing"); changedProperties.count(key))
		{
			_properties.legacyPairing = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("Paired"); changedProperties.count(key))
		{
			_properties.paired = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("Modalias"); changedProperties.count(key))
		{
			_properties.modalias = parseModalias(changedProperties.at(key).get<std::string>());
		}
		if (const auto key = sdbus::MemberName("Name"); changedProperties.count(key))
		{
			_properties.name = changedProperties.at(key).get<std::string>();
		}
		if (const auto key = sdbus::MemberName("ServiceData"); changedProperties.count(key))
		{
			_properties.serviceData =
				changedProperties.at(key).get<std::map<std::string, sdbus::Variant>>();
		}
		if (const auto key = sdbus::MemberName("RSSI"); changedProperties.count(key))
		{
			_properties.rssi = changedProperties.at(key).get<std::int16_t>();
			spdlog::debug("RSSI: {}", _properties.rssi);
		}
		if (const auto key = sdbus::MemberName("ServicesResolved"); changedProperties.count(key))
		{
			_properties.servicesResolved = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("Trusted"); changedProperties.count(key))
		{
			_properties.trusted = changedProperties.at(key).get<bool>();
		}
		if (const auto key = sdbus::MemberName("UUIDs"); changedProperties.count(key))
		{
			_properties.uuids = changedProperties.at(key).get<std::vector<std::string>>();
		}
#if 0
    Utils::print_changed_properties(interfaceName, changedProperties,
                                    invalidatedProperties);
#endif
	}
};

#endif // BLUETOOTH_DEVICE_H_