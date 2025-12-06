#ifndef BLUETOOTH_AGENT_MANAGER_H_
#define BLUETOOTH_AGENT_MANAGER_H_

#include <bluetooth/proxy/agent_manager_proxy.h>

class AgentManager : public sdbus::ProxyInterfaces<org::bluez::AgentManager1_proxy>
{
public:
	static constexpr auto destination = "org.bluez";
	static constexpr auto objectPath = "/org/bluez";

	AgentManager(sdbus::IConnection& connection)
		: ProxyInterfaces(connection,
						  sdbus::ServiceName(destination),
						  sdbus::ObjectPath(objectPath))
	{
		registerProxy();
	}

	~AgentManager()
	{
		unregisterProxy();
	}
};



#endif // BLUETOOTH_AGENT_MANAGER_H_