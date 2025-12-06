
#include <bluetooth/agent.h>

Agent::Agent(sdbus::IConnection& connection, sdbus::ObjectPath objectPath)
	: AdaptorInterfaces(connection, objectPath)
{
	registerAdaptor();
}

Agent::~Agent() { unregisterAdaptor(); }

void Agent::setPinRequestCallback(PinRequestCallback callback)
{
	_pinRequestCallback = std::move(callback);
}

void Agent::setConfirmationCallback(ConfirmationCallback callback)
{
	_confirmationCallback = std::move(callback);
}

std::string Agent::onRequestPincode(sdbus::ObjectPath objectPath)
{
	spdlog::info("请求PIN码，设备路径: {}", objectPath);

	if (_pinRequestCallback)
		return _pinRequestCallback(objectPath);

	throw sdbus::Error(sdbus::Error::Name("org.bluez.Error.Rejected"), "PIN code request rejected");
}


void Agent::onDisplayPincode(sdbus::ObjectPath objectPath, const std::string& pincode)
{
	spdlog::info("显示PIN码，设备路径: {}, PIN码: {}", objectPath, pincode);
}

uint32_t Agent::onRequestPasskey(sdbus::ObjectPath objectPath)
{
	spdlog::info("请求Passkey，设备路径: {}", objectPath);

	if (_pinRequestCallback)
	{
		auto pincode = _pinRequestCallback(objectPath);
		return std::stoul(pincode);
	}

	throw sdbus::Error(sdbus::Error::Name("org.bluez.Error.Rejected"), "Passkey request rejected");
}

void Agent::onDisplayPasskey(sdbus::ObjectPath objectPath, uint32_t passkey, uint16_t entered)
{
	spdlog::info("显示Passkey，设备路径: {}, Passkey: {}, 已输入: {}", objectPath, passkey, entered);
}

void Agent::onRequestConfirmation(sdbus::ObjectPath objectPath, uint32_t passkey)
{
	spdlog::info("请求确认，设备路径: {}, Passkey: {}", objectPath, passkey);

	if (_confirmationCallback)
	{
		bool accepted = _confirmationCallback(objectPath, passkey);
		if (!accepted)
		{
			throw sdbus::Error(sdbus::Error::Name("org.bluez.Error.Rejected"),
							   "User rejected paring");
		}

		return;
	}

	throw sdbus::Error(sdbus::Error::Name("org.bluez.Error.Rejected"),
					   "No confirmation callback set");
}
