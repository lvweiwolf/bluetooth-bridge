#ifndef BLUETOOTH_UTILS_H
#define BLUETOOTH_UTILS_H

#include <defines.h>
#include <sdbus-c++/sdbus-c++.h>
#include <spdlog/spdlog.h>

#include <sstream>


class CORE_API Utils
{
public:
	static void appendProperty(const sdbus::Variant& value, std::ostringstream& os);

	static void appendProperties(const std::map<sdbus::MemberName, sdbus::Variant>& props,
								 std::ostringstream& os);

	static void appendProperties(const std::map<std::string, sdbus::Variant>& props,
								 std::ostringstream& os);

	static void printChangedProperties(
		const sdbus::InterfaceName& interfaceName,
		const std::map<sdbus::PropertyName, sdbus::Variant>& changedProperties,
		const std::vector<sdbus::PropertyName>& invalidatedProperties);

	static std::vector<std::string> listNames(sdbus::IConnection& connection);

	static bool isServicePresent(const std::vector<std::string>& dbus_interfaces,
								 const std::string_view& service);

	static std::string parseDescriptionJson(const std::string& json);

private:
	static const std::unordered_map<std::string_view,
									std::function<void(const sdbus::Variant&, std::ostringstream&)>>
		_typemap;
};

#endif // BLUETOOTH_UTILS_H