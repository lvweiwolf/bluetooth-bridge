#ifndef BLUETOOTH_PROXY_ADAPTER_PROXY_H_
#define BLUETOOTH_PROXY_ADAPTER_PROXY_H_

#include <sdbus-c++/sdbus-c++.h>

namespace org::bluez {

	class Adapter1_proxy
	{
	public:
		static constexpr auto INTERFACE_NAME = "org.bluez.Adapter1";

		// 设备发现、连接
		void startDiscovery() { _proxy.callMethod("StartDiscovery").onInterface(INTERFACE_NAME); }

		void set_discovery_filter(const std::map<std::string, sdbus::Variant>& properties)
		{
			_proxy.callMethod("SetDiscoveryFilter")
				.onInterface(INTERFACE_NAME)
				.withArguments(properties);
		}

		void stopDiscovery() { _proxy.callMethod("StopDiscovery").onInterface(INTERFACE_NAME); }

		void removeDevice(const sdbus::ObjectPath& device)
		{
			_proxy.callMethod("RemoveDevice").onInterface(INTERFACE_NAME).withArguments(device);
		}

		std::vector<std::string> getDiscoveryFilters()
		{
			std::vector<std::string> result;
			_proxy.callMethod("GetDiscoveryFilters")
				.onInterface(INTERFACE_NAME)
				.storeResultsTo(result);
			return result;
		}

		void connectDevice(const std::map<std::string, sdbus::Variant>& properties)
		{
			_proxy.callMethod("ConnectDevice")
				.onInterface(INTERFACE_NAME)
				.withArguments(properties);
		}

		// 设备属性
		std::string address()
		{
			return _proxy.getProperty("Address").onInterface(INTERFACE_NAME).get<std::string>();
		}

		std::string addressType()
		{
			return _proxy.getProperty("AddressType").onInterface(INTERFACE_NAME).get<std::string>();
		}

		std::string name()
		{
			return _proxy.getProperty("Name").onInterface(INTERFACE_NAME).get<std::string>();
		}

		std::string alias()
		{
			return _proxy.getProperty("Alias").onInterface(INTERFACE_NAME).get<std::string>();
		}

		void setAlias(const std::string& value)
		{
			_proxy.setProperty("Alias").onInterface(INTERFACE_NAME).toValue(value);
		}

		uint32_t classType()
		{
			return _proxy.getProperty("Class").onInterface(INTERFACE_NAME).get<uint32_t>();
		}

		bool powered()
		{
			return _proxy.getProperty("Powered").onInterface(INTERFACE_NAME).get<bool>();
		}

		void setPowered(const bool& value)
		{
			_proxy.setProperty("Powered").onInterface(INTERFACE_NAME).toValue(value);
		}

		bool discoverable()
		{
			return _proxy.getProperty("Discoverable").onInterface(INTERFACE_NAME).get<bool>();
		}

		void setDiscoverable(const bool& value)
		{
			_proxy.setProperty("Discoverable").onInterface(INTERFACE_NAME).toValue(value);
		}

		uint32_t discoverableTimeout()
		{
			return _proxy.getProperty("DiscoverableTimeout")
				.onInterface(INTERFACE_NAME)
				.get<uint32_t>();
		}

		void setDiscoverableTimeout(const uint32_t& value)
		{
			_proxy.setProperty("DiscoverableTimeout").onInterface(INTERFACE_NAME).toValue(value);
		}

		bool pairable()
		{
			return _proxy.getProperty("Pairable").onInterface(INTERFACE_NAME).get<bool>();
		}

		void setPairable(const bool& value)
		{
			_proxy.setProperty("Pairable").onInterface(INTERFACE_NAME).toValue(value);
		}

		uint32_t pairableTimeout()
		{
			return _proxy.getProperty("PairableTimeout")
				.onInterface(INTERFACE_NAME)
				.get<uint32_t>();
		}

		void setPairableTimeout(const uint32_t& value)
		{
			_proxy.setProperty("PairableTimeout").onInterface(INTERFACE_NAME).toValue(value);
		}

		bool discovering()
		{
			return _proxy.getProperty("Discovering").onInterface(INTERFACE_NAME).get<bool>();
		}

		std::vector<std::string> uuids()
		{
			return _proxy.getProperty("UUIDs")
				.onInterface(INTERFACE_NAME)
				.get<std::vector<std::string>>();
		}

		std::string modalias()
		{
			return _proxy.getProperty("Modalias").onInterface(INTERFACE_NAME).get<std::string>();
		}


	protected:
		explicit Adapter1_proxy(sdbus::IProxy& proxy) : _proxy(proxy) {}

		~Adapter1_proxy() = default;

		void registerProxy() {}

	private:
		Adapter1_proxy(const Adapter1_proxy&) = delete;
		Adapter1_proxy(Adapter1_proxy&&) = delete;
		Adapter1_proxy& operator=(const Adapter1_proxy&) = delete;
		Adapter1_proxy& operator=(Adapter1_proxy&&) = delete;


		sdbus::IProxy& _proxy;
	};
}


#endif // BLUETOOTH_PROXY_ADAPTER_PROXY_H_