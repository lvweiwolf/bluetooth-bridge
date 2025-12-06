#ifndef BLUETOOTH_PROXY_PROFILE_MANAGER_PROXY_H_
#define BLUETOOTH_PROXY_PROFILE_MANAGER_PROXY_H_


#include <sdbus-c++/sdbus-c++.h>
#include <string>

namespace org::bluez {

	class ProfileManager1_proxy
	{
	public:
		static constexpr auto INTERFACE_NAME = "org.bluez.ProfileManager1";

	protected:
		explicit ProfileManager1_proxy(sdbus::IProxy& proxy) : _proxy(proxy) {}

		~ProfileManager1_proxy() = default;

		void registerProxy() {}

	public:
		void registerProfile(const sdbus::ObjectPath& profile,
							  const std::string& uuid,
							  const std::map<std::string, sdbus::Variant>& options)
		{
			_proxy.callMethod("RegisterProfile")
				.onInterface(INTERFACE_NAME)
				.withArguments(profile, uuid, options);
		}

		void unregisterProfile(const sdbus::ObjectPath& profile)
		{
			_proxy.callMethod("UnregisterProfile")
				.onInterface(INTERFACE_NAME)
				.withArguments(profile);
		}

	private:
		ProfileManager1_proxy(const ProfileManager1_proxy&) = delete;
		ProfileManager1_proxy(ProfileManager1_proxy&&) = delete;
		ProfileManager1_proxy& operator=(const ProfileManager1_proxy&) = delete;
		ProfileManager1_proxy& operator=(ProfileManager1_proxy&&) = delete;


		sdbus::IProxy& _proxy;
	};

} // namespace org::bluez
#endif // BLUETOOTH_PROXY_PROFILE_MANAGER_PROXY_H_