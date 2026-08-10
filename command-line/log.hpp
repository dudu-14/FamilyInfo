// log.hpp
#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

namespace FamilyInfo::Log
{

	// 日志级别（数值越小越严重）
	enum class LogLevel {
		FATAL = 0,
		ERROR = 1,
		WARNING = 2,
		INFO = 3,
		DEBUG = 4
	};

	// 当前生效的日志级别（默认 INFO）
	inline LogLevel g_logLevel = LogLevel::INFO;
	// 全局日志文件流
	inline std::ofstream g_logFile;

	// 初始化日志系统（在 main 开头调用一次）
	inline void initLog(const std::string& filePath = "logs/system.log",
		LogLevel level = LogLevel::INFO) {
		g_logLevel = level;
		if (!filePath.empty()) {
			std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
			g_logFile.open(filePath, std::ios::app);
		}
	}

	// 内部写日志函数
	inline void writeLog(LogLevel level, const std::string& prefix, const std::string& msg) {
		if (level > g_logLevel) return;  // 高于当前级别则不输出

		std::string line = "[" + prefix + "] " + msg + "\n";

		// 输出到控制台
		if (level <= LogLevel::ERROR) {
			std::cerr << line;
		}
		else {
			std::cout << line;
		}

		// 输出到文件
		if (g_logFile.is_open()) {
			g_logFile << line;
			g_logFile.flush();
		}
	}

	// ---------- 对外接口 ----------
	inline void logFatal(const std::string& msg) {
		writeLog(LogLevel::FATAL, "FATAL", msg);
		throw std::runtime_error(msg); //抛出异常
	}
	inline void logError(const std::string& msg) { writeLog(LogLevel::ERROR, "ERROR", msg); }
	inline void logWarn(const std::string& msg) { writeLog(LogLevel::WARNING, "WARNING", msg); }
	inline void logInfo(const std::string& msg) { writeLog(LogLevel::INFO, "INFO", msg); }
	inline void logDebug(const std::string& msg) { writeLog(LogLevel::DEBUG, "DEBUG", msg); }


	inline void setLogLevel(LogLevel level) { g_logLevel = level; }

}