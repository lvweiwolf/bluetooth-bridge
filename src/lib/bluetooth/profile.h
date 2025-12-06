#ifndef BLUETOOTH_PROFILE_H_
#define BLUETOOTH_PROFILE_H_


#include <defines.h>
#include <bluetooth/object/profile_adaptor.h>
#include <string>
#include <functional>

class CORE_API Profile : public sdbus::AdaptorInterfaces<org::bluez::Profile1_adaptor>
{
public:
	Profile(sdbus::IConnection& connection, sdbus::ObjectPath objectPath);

	~Profile();

	uint16_t getRfcommChannel() const;

	void setRfcommChannel(uint16_t rfcommChanel);

private:
	void onNewConnection(const sdbus::ObjectPath& device,
						 const sdbus::UnixFd& fd,
						 const std::map<std::string, sdbus::Variant>& properties) override;

	void onRequestDisconnection(const sdbus::ObjectPath& device) override;

	void onCancel() override;

	void onRelease() override;

private:
	uint16_t _rfcommChannel = 1;
};


#endif // BLUETOOTH_PROFILE_H_