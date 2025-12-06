#ifndef BLUETOOTH_PROFILE_MANAGER_H_
#define BLUETOOTH_PROFILE_MANAGER_H_

#include <defines.h>
#include <bluetooth/proxy/profile_manager_proxy.h>

class CORE_API ProfileManager : public sdbus::ProxyInterfaces<org::bluez::ProfileManager1_proxy>
{
public:
	static constexpr auto destination = "org.bluez";
	static constexpr auto objectPath = "/org/bluez";

	ProfileManager(sdbus::IConnection& connection)
		: ProxyInterfaces(connection,
						  sdbus::ServiceName(destination),
						  sdbus::ObjectPath(objectPath))
	{
		registerProxy();
	}

	~ProfileManager() { unregisterProxy(); }
};


#endif // BLUETOOTH_PROFILE_MANAGER_H_