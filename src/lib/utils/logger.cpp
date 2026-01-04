#include <utils/logger.h>
#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<LevelSpecificLogger> LevelSpecificLogger::_instance =
	std::make_shared<LevelSpecificLogger>();

LevelSpecificLogger::LevelSpecificLogger()
{
	_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	_console_sink->set_level(spdlog::level::info);


	// 为每个级别创建独立的文件sink
	createLevelLogger(spdlog::level::debug, "logs/debug.log");
	createLevelLogger(spdlog::level::info, "logs/info.log");
	createLevelLogger(spdlog::level::warn, "logs/warn.log");
	createLevelLogger(spdlog::level::err, "logs/error.log");

	spdlog::flush_on(spdlog::level::warn);
	spdlog::flush_every(std::chrono::seconds(3));

}

LevelSpecificLogger::~LevelSpecificLogger()
{
	spdlog::shutdown();
}

void LevelSpecificLogger::createLevelLogger(spdlog::level::level_enum level,
											const std::string& filename)
{
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
	file_sink->set_level(level);

	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(file_sink);

	if (level != spdlog::level::debug)
		sinks.push_back(_console_sink);

	auto logger = std::make_shared<spdlog::logger>(
		fmt::format("{}_logger", spdlog::level::to_string_view(level)),
		sinks.begin(),
		sinks.end());

	logger->set_level(level);
	spdlog::register_logger(logger);
	_loggers[level] = logger;
}
