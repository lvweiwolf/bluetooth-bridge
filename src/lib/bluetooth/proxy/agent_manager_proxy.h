#ifndef BLUETOOTH_PROXY_AGENT_MANAGER_PROXY_H_
#define BLUETOOTH_PROXY_AGENT_MANAGER_PROXY_H_


#include <sdbus-c++/sdbus-c++.h>
#include <string>

namespace org::bluez {

	class AgentManager1_proxy
	{
	public:
		static constexpr auto INTERFACE_NAME = "org.bluez.AgentManager1";

	protected:
		explicit AgentManager1_proxy(sdbus::IProxy& proxy) : _proxy(proxy) {}

		~AgentManager1_proxy() = default;

		void registerProxy() {}

	public:
		void registerAgent(const sdbus::ObjectPath& agent, const std::string& capability)
		{
			_proxy.callMethod("RegisterAgent")
				.onInterface(INTERFACE_NAME)
				.withArguments(agent, capability);
		}

		void unregisterAgent(const sdbus::ObjectPath& agent)
		{
			_proxy.callMethod("UnregisterAgent").onInterface(INTERFACE_NAME).withArguments(agent);
		}

		void requestDefaultAgent(const sdbus::ObjectPath& agent)
		{
			_proxy.callMethod("RequestDefaultAgent")
				.onInterface(INTERFACE_NAME)
				.withArguments(agent);
		}

	private:
		AgentManager1_proxy(const AgentManager1_proxy&) = delete;
		AgentManager1_proxy(AgentManager1_proxy&&) = delete;
		AgentManager1_proxy& operator=(const AgentManager1_proxy&) = delete;
		AgentManager1_proxy& operator=(AgentManager1_proxy&&) = delete;

		
		sdbus::IProxy& _proxy;
	};

} // namespace org::bluez

#endif  // BLUETOOTH_PROXY_AGENT_MANAGER_PROXY_H_