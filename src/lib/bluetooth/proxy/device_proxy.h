#ifndef BLUETOOTH_PROXY_DEVICE_PROXY_H_
#define BLUETOOTH_PROXY_DEVICE_PROXY_H_

#include <sdbus-c++/sdbus-c++.h>
#include <string>


namespace org::bluez {

	class Device1_proxy
	{
	public:
		static constexpr auto INTERFACE_NAME = "org.bluez.Device1";

	protected:
		explicit Device1_proxy(sdbus::IProxy& proxy) : _proxy(proxy) {}

		~Device1_proxy() = default;

		void registerProxy() {}

	public:
		// 设备方法
		void connect() { _proxy.callMethod("Connect").onInterface(INTERFACE_NAME); }

		void connect(uint64_t timeout)
		{
			auto method = _proxy.createMethodCall(sdbus::InterfaceName(INTERFACE_NAME),
												  sdbus::MethodName("Connect"));
			
			_proxy.callMethod(method, timeout);
		}

		void disconnect() { _proxy.callMethod("Disconnect").onInterface(INTERFACE_NAME); }

		void connectProfile(const std::string& uuid)
		{
			_proxy.callMethod("ConnectProfile").onInterface(INTERFACE_NAME).withArguments(uuid);
		}

		void disconnectProfile(const std::string& uuid)
		{
			_proxy.callMethod("DisconnectProfile").onInterface(INTERFACE_NAME).withArguments(uuid);
		}

		void pair() { _proxy.callMethod("Pair").onInterface(INTERFACE_NAME); }

		void pair(uint64_t timeout)
		{
			auto method = _proxy.createMethodCall(sdbus::InterfaceName(INTERFACE_NAME),
												  sdbus::MethodName("Pair"));

			_proxy.callMethod(method, timeout);
		}

		void cancelPairing() { _proxy.callMethod("CancelPairing").onInterface(INTERFACE_NAME); }

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

		std::string icon()
		{
			return _proxy.getProperty("Icon").onInterface(INTERFACE_NAME).get<std::string>();
		}

		uint32_t classType()
		{
			return _proxy.getProperty("Class").onInterface(INTERFACE_NAME).get<uint32_t>();
		}

		uint16_t appearance()
		{
			return _proxy.getProperty("Appearance").onInterface(INTERFACE_NAME).get<uint16_t>();
		}

		std::vector<std::string> uuids()
		{
			return _proxy.getProperty("UUIDs")
				.onInterface(INTERFACE_NAME)
				.get<std::vector<std::string>>();
		}

		bool paired()
		{
			return _proxy.getProperty("Paired").onInterface(INTERFACE_NAME).get<bool>();
		}

		bool connected()
		{
			return _proxy.getProperty("Connected").onInterface(INTERFACE_NAME).get<bool>();
		}

		bool trusted()
		{
			return _proxy.getProperty("Trusted").onInterface(INTERFACE_NAME).get<bool>();
		}

		void setTrusted(const bool& value)
		{
			_proxy.setProperty("Trusted").onInterface(INTERFACE_NAME).toValue(value);
		}

		bool blocked()
		{
			return _proxy.getProperty("Blocked").onInterface(INTERFACE_NAME).get<bool>();
		}

		void setBlocked(const bool& value)
		{
			_proxy.setProperty("Blocked").onInterface(INTERFACE_NAME).toValue(value);
		}

		bool wakeAllowed()
		{
			return _proxy.getProperty("WakeAllowed").onInterface(INTERFACE_NAME).get<bool>();
		}

		void setWakeAllowed(const bool& value)
		{
			_proxy.setProperty("WakeAllowed").onInterface(INTERFACE_NAME).toValue(value);
		}

		std::string alias()
		{
			return _proxy.getProperty("Alias").onInterface(INTERFACE_NAME).get<std::string>();
		}

		void setAlias(const std::string& value)
		{
			_proxy.setProperty("Alias").onInterface(INTERFACE_NAME).toValue(value);
		}

		sdbus::ObjectPath adapter()
		{
			return _proxy.getProperty("Adapter")
				.onInterface(INTERFACE_NAME)
				.get<sdbus::ObjectPath>();
		}

		bool legacyPairing()
		{
			return _proxy.getProperty("LegacyPairing").onInterface(INTERFACE_NAME).get<bool>();
		}

		std::string modalias()
		{
			return _proxy.getProperty("Modalias").onInterface(INTERFACE_NAME).get<std::string>();
		}

		int16_t rssi()
		{
			return _proxy.getProperty("RSSI").onInterface(INTERFACE_NAME).get<int16_t>();
		}

		int16_t txPower()
		{
			return _proxy.getProperty("TxPower").onInterface(INTERFACE_NAME).get<int16_t>();
		}

		std::map<uint16_t, sdbus::Variant> manufacturerData()
		{
			return _proxy.getProperty("ManufacturerData")
				.onInterface(INTERFACE_NAME)
				.get<std::map<uint16_t, sdbus::Variant>>();
		}

		std::map<std::string, sdbus::Variant> serviceData()
		{
			return _proxy.getProperty("ServiceData")
				.onInterface(INTERFACE_NAME)
				.get<std::map<std::string, sdbus::Variant>>();
		}

		bool servicesResolved()
		{
			return _proxy.getProperty("ServicesResolved").onInterface(INTERFACE_NAME).get<bool>();
		}

		std::vector<bool> advertisingFlags()
		{
			return _proxy.getProperty("AdvertisingFlags")
				.onInterface(INTERFACE_NAME)
				.get<std::vector<bool>>();
		}

		std::map<uint8_t, sdbus::Variant> advertisingData()
		{
			return _proxy.getProperty("AdvertisingData")
				.onInterface(INTERFACE_NAME)
				.get<std::map<uint8_t, sdbus::Variant>>();
		}


	private:
		Device1_proxy(const Device1_proxy&) = delete;
		Device1_proxy(Device1_proxy&&) = delete;
		Device1_proxy& operator=(const Device1_proxy&) = delete;
		Device1_proxy& operator=(Device1_proxy&&) = delete;

		sdbus::IProxy& _proxy;
	};

} // namespace org::bluez
  //
#endif // BLUETOOTH_PROXY_DEVICE_PROXY_H_