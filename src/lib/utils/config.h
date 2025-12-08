#ifndef UTILS_CONFIG_H_
#define UTILS_CONFIG_H_

#include <json/json.h>
#include <string>

class JsonConfig
{
public:
	JsonConfig(const std::string& filename);
	virtual ~JsonConfig();

	bool load();
	
	bool save(const std::string& filename);

	Json::Value& getRoot() { return _root; }

	std::string getString(const std::string& key, const std::string& value = "");

	int getInt(const std::string& key, int value = 0);

	double getDouble(const std::string& key, double value = 0.0);

	bool getBool(const std::string& key, bool value = false);

	Json::Value getArray(const std::string& key);

private:
	Json::Value getValueByPath(const std::string& key);
	
	std::string _configPath;
	Json::Value _root;
};

#endif
