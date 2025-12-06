#ifndef BLUETOOTH_AGENT_H_
#define BLUETOOTH_AGENT_H_

#include <bluetooth/object/agent_adaptor.h>
#include <string>
#include <functional>

class Agent : public sdbus::AdaptorInterfaces<org::bluez::Agent1_adaptor>
{
public:
	Agent(sdbus::IConnection& connection, sdbus::ObjectPath objectPath);

	~Agent();

	// 回调设置
	using PinRequestCallback = std::function<std::string(const std::string&)>;
	using ConfirmationCallback = std::function<bool(const std::string&, uint32_t)>;

	void setPinRequestCallback(PinRequestCallback callback);
	void setConfirmationCallback(ConfirmationCallback callback);

private:
	// 代理接口
	std::string onRequestPincode(sdbus::ObjectPath objectPath) override;

	void onDisplayPincode(sdbus::ObjectPath objectPath, const std::string& pincode) override;

	uint32_t onRequestPasskey(sdbus::ObjectPath objectPath) override;

	void onDisplayPasskey(sdbus::ObjectPath objectPath,
						  uint32_t passkey,
						  uint16_t entered) override;

	void onRequestConfirmation(sdbus::ObjectPath objectPath, uint32_t passkey) override;

private:
	PinRequestCallback _pinRequestCallback;
	ConfirmationCallback _confirmationCallback;
};


#endif // BLUETOOTH_AGENT_H_