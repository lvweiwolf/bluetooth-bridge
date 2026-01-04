#ifndef BLUETOOTH_UTILS_LOGGER_H_
#define BLUETOOTH_UTILS_LOGGER_H_

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <unordered_map>



class LevelSpecificLogger
{
public:
	LevelSpecificLogger();

	~LevelSpecificLogger();

	template <typename... Args>
	inline void log(spdlog::level::level_enum lvl,
					spdlog::format_string_t<Args...> fmt,
					Args&&... args)
	{
		auto it = _loggers.find(lvl);
		if (it != _loggers.end())
			it->second->log(lvl, fmt, std::forward<Args>(args)...);
	}


	template <typename T>
	void log(spdlog::level::level_enum lvl, const T& msg)
	{
		auto it = _loggers.find(lvl);
		if (it != _loggers.end())
			it->second->log(lvl, msg);
	}

	static auto getInstance() { return _instance; }

private:
	void createLevelLogger(spdlog::level::level_enum level, const std::string& logger_name);

private:
	static std::shared_ptr<LevelSpecificLogger> _instance;

	std::unordered_map<spdlog::level::level_enum, std::shared_ptr<spdlog::logger>> _loggers;
	std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> _console_sink;
};

#define LOG_DEBUG(...) LevelSpecificLogger::getInstance()->log(spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...) LevelSpecificLogger::getInstance()->log(spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...) LevelSpecificLogger::getInstance()->log(spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...) LevelSpecificLogger::getInstance()->log(spdlog::level::err, __VA_ARGS__)

#endif // BLUETOOTH_UTILS_LOGGER_H_