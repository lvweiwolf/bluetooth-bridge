#ifndef BLUETOOTH_OBJECT_AGENT_ADAPTOR_H_
#define BLUETOOTH_OBJECT_AGENT_ADAPTOR_H_

#include <utils/logger.h>

#include <sdbus-c++/sdbus-c++.h>

namespace org::bluez {
	class Agent1_adaptor
	{
	public:
		static constexpr auto INTERFACE_NAME = "org.bluez.Agent1";

	protected:
		explicit Agent1_adaptor(sdbus::IObject& object) : _object(object) {}

		~Agent1_adaptor() = default;

		void registerAdaptor()
		{
			try
			{
				// 注册代理接口
				_object
					.addVTable(sdbus::registerMethod("RequestPinCode")
								   .withInputParamNames("device")
								   .withOutputParamNames("pincode")
								   .implementedAs([this](sdbus::ObjectPath device) {
									   return onRequestPincode(device);
								   }))
					.forInterface(INTERFACE_NAME);


				_object
					.addVTable(sdbus::registerMethod("DisplayPinCode")
								   .withInputParamNames("device", "pincode")
								   .implementedAs([this](sdbus::ObjectPath device,
														 const std::string& pincode) {
									   onDisplayPincode(device, pincode);
								   }))
					.forInterface(INTERFACE_NAME);

				_object
					.addVTable(sdbus::registerMethod("RequestPasskey")
								   .withInputParamNames("device")
								   .withOutputParamNames("passkey")
								   .implementedAs([this](sdbus::ObjectPath device) {
									   return onRequestPasskey(device);
								   }))
					.forInterface(INTERFACE_NAME);

				_object
					.addVTable(sdbus::registerMethod("DisplayPasskey")
								   .withInputParamNames("device", "passkey", "entered")
								   .implementedAs([this](sdbus::ObjectPath device,
														 uint32_t passkey,
														 uint16_t entered) {
									   onDisplayPasskey(device, passkey, entered);
								   }))
					.forInterface(INTERFACE_NAME);

				_object
					.addVTable(
						sdbus::registerMethod("RequestConfirmation")
							.withInputParamNames("device", "passkey")
							.implementedAs([this](sdbus::ObjectPath device, uint32_t passkey) {
								onRequestConfirmation(device, passkey);
							}))
					.forInterface(INTERFACE_NAME);
			}
			catch (const sdbus::Error& e)
			{
				LOG_ERROR("注册蓝牙代理接口失败 - {}", e.what());
			}
		}

		virtual std::string onRequestPincode(sdbus::ObjectPath device_path) = 0;

		virtual void onDisplayPincode(sdbus::ObjectPath device_path,
									  const std::string& pin_code) = 0;

		virtual uint32_t onRequestPasskey(sdbus::ObjectPath device_path) = 0;

		virtual void onDisplayPasskey(sdbus::ObjectPath device_path,
									  uint32_t passkey,
									  uint16_t entered) = 0;

		virtual void onRequestConfirmation(sdbus::ObjectPath device_path, uint32_t passkey) = 0;

	private:
		Agent1_adaptor(const Agent1_adaptor&) = delete;
		Agent1_adaptor(Agent1_adaptor&&) = delete;
		Agent1_adaptor& operator=(const Agent1_adaptor&) = delete;
		Agent1_adaptor& operator=(Agent1_adaptor&&) = delete;

		sdbus::IObject& _object;
	};
} // namespace org::bluez

#endif // BLUETOOTH_OBJECT_AGENT_ADAPTOR_H_