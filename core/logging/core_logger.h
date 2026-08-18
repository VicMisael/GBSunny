#pragma once

#include <string_view>

namespace logging {
	enum class LogLevel {
		Debug,
		Info,
		Warning,
		Error,
	};

	class CoreLogger {
	public:
		virtual ~CoreLogger() = default;

		virtual void log(LogLevel level, std::string_view message) = 0;

		void debug(std::string_view message) { log(LogLevel::Debug, message); }
		void info(std::string_view message) { log(LogLevel::Info, message); }
		void warning(std::string_view message) { log(LogLevel::Warning, message); }
		void error(std::string_view message) { log(LogLevel::Error, message); }
	};

	class NullCoreLogger final : public CoreLogger {
	public:
		void log(LogLevel, std::string_view) override {}
	};
}
