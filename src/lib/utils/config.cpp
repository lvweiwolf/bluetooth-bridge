#include <utils/config.h>
#include <fstream>
#include <sstream>


JsonConfig::JsonConfig(const std::string& filename) : _configPath(filename) {}

JsonConfig::~JsonConfig() {}

bool JsonConfig::load()
{
	std::ifstream file(_configPath);
	if (!file)
		return false;

	Json::CharReaderBuilder readerBuilder;
	std::string errs;
	bool success = Json::parseFromStream(readerBuilder, file, &_root, &errs);
	file.close();

	return success;
}

Json::Value JsonConfig::getValueByPath(const std::string& key)
{
	std::istringstream iss(key);
	std::string token;
	Json::Value current = _root;

	while (std::getline(iss, token, '.') && !current.isNull())
	{
		if (current.isObject())
		{
			current = current[token];
		}
		else
		{
			return Json::Value::null; // 路径中途不是对象，返回空值
		}
	}
	
	return current; // 返回找到的值或空值
}

std::string JsonConfig::getString(const std::string& key, const std::string& defaultValue)
{
	Json::Value value = getValueByPath(key);
	return (value.isString() ? value.asString() : defaultValue);
}

int JsonConfig::getInt(const std::string& key, int defaultValue)
{
    Json::Value value = getValueByPath(key);
    return (value.isInt() ? value.asInt() : defaultValue);
}

bool JsonConfig::getBool(const std::string& key, bool defaultValue)
{
    Json::Value value = getValueByPath(key);
    return (value.isBool() ? value.asBool() : defaultValue);
}

double JsonConfig::getDouble(const std::string& key, double defaultValue)
{
    Json::Value value = getValueByPath(key);
    return (value.isDouble() ? value.asDouble() : defaultValue);
}

Json::Value JsonConfig::getArray(const std::string& key)
{
    return getValueByPath(key);
}