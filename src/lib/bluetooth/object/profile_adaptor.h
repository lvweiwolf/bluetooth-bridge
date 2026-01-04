#ifndef BLUETOOTH_OBJECT_PROFILE_ADAPTOR_H_
#define BLUETOOTH_OBJECT_PROFILE_ADAPTOR_H_

#include <utils/logger.h>

#include <sdbus-c++/sdbus-c++.h>

namespace org::bluez {
	class Profile1_adaptor
	{
	public:
		static constexpr auto INTERFACE_NAME = "org.bluez.Profile1";

	protected:
		explicit Profile1_adaptor(sdbus::IObject& object) : _object(object) {}

		~Profile1_adaptor() = default;

		void registerAdaptor()
		{
			try
			{
				// 注册方法
				_object
					.addVTable(
						sdbus::registerMethod("NewConnection")
							.withInputParamNames("device", "fd", "options")
							.implementedAs(
								[this](const sdbus::ObjectPath& device,
									   const sdbus::UnixFd& fd,
									   const std::map<std::string, sdbus::Variant>& properties) {
									this->onNewConnection(device, fd, properties);
								}))
					.forInterface(INTERFACE_NAME);


				_object
					.addVTable(sdbus::registerMethod("RequestDisconnection")
								   .withInputParamNames("device")
								   .implementedAs([this](const sdbus::ObjectPath& device) {
									   this->onRequestDisconnection(device);
								   }))
					.forInterface(INTERFACE_NAME);

				_object
					.addVTable(sdbus::registerMethod("Release").implementedAs(
						[this]() { this->onRelease(); }))
					.forInterface(INTERFACE_NAME);

				_object
					.addVTable(sdbus::registerMethod("Cancel").implementedAs(
						[this]() { this->onCancel(); }))
					.forInterface(INTERFACE_NAME);
			}
			catch (const sdbus::Error& e)
			{
				LOG_ERROR("注册蓝牙配置接口失败 - {}", e.what());
			}
		}

		virtual void onNewConnection(const sdbus::ObjectPath& device,
									 const sdbus::UnixFd& fd,
									 const std::map<std::string, sdbus::Variant>& properties) = 0;

		virtual void onRequestDisconnection(const sdbus::ObjectPath& device) = 0;


		virtual void onCancel() = 0;

		virtual void onRelease() = 0;

	private:
		Profile1_adaptor(const Profile1_adaptor&) = delete;
		Profile1_adaptor(Profile1_adaptor&&) = delete;
		Profile1_adaptor& operator=(const Profile1_adaptor&) = delete;
		Profile1_adaptor& operator=(Profile1_adaptor&&) = delete;

		sdbus::IObject& _object;
	};
} // namespace org::bluez


#endif // BLUETOOTH_OBJECT_PROFILE_ADAPTOR_H_
