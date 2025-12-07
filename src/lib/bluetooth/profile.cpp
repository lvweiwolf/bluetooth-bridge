#include <bluetooth/profile.h>

Profile::Profile(sdbus::IConnection& connection, sdbus::ObjectPath objectPath)
	: AdaptorInterfaces(connection, objectPath)
{
	registerAdaptor();
}

Profile::~Profile() { unregisterAdaptor(); }

uint16_t Profile::getRfcommChannel() const { return _rfcommChannel; }

void Profile::setRfcommChannel(uint16_t channel) { _rfcommChannel = channel; }


void Profile::onNewConnection(const sdbus::ObjectPath& device,
							  const sdbus::UnixFd& fd,
							  const std::map<std::string, sdbus::Variant>& properties)
{

}

void Profile::onRequestDisconnection(const sdbus::ObjectPath& device)
{

}

void Profile::onCancel()
{

}

void Profile::onRelease()
{

}
