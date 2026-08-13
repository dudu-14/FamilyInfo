// config.hpp
#pragma once
#include <filesystem>
#include <iostream>
#include <fstream>
#include "family.hpp"
#include "json.hpp"
#include "log.hpp"

using json = nlohmann::json;

namespace FamilyInfo::Config
{
	/// <summary>
	/// 检测配置文件是否存在
	/// </summary>
	/// <returns>bool类型，存在返回true，否则返回false</returns>
	inline bool checkFileExists(std::filesystem::path _configPath = "data/config.json")
	{
		return std::filesystem::exists(_configPath);
	}


	/// <summary>
	/// 初始化配置文件
	/// </summary>
	/// <returns>正常返回true,失败返回false</returns>
	//bool init()
	//{
	//	if (checkFileExists())
	//	{
	//		return true;
	//	}
	//	std::filesystem::path configPath = "data/config.json";
	//	std::filesystem::create_directories(configPath.parent_path());
	//	std::ofstream configFile(configPath);
	//	if (!configFile.is_open())
	//	{
	//		std::cerr << "[ERROR] 无法创建配置文件" << std::endl;
	//		Log::logError("无法创建配置文件");
	//		return false;
	//	}
	//	configFile.close();
	//	return true;
	//}

	/// <summary>配置结构体</summary>
	struct AppConfig {
		// paths
		std::string dataFile = "data/family_data.json";
		std::string logFile = "logs/system.log";
		std::string debugLogFile = "logs/debug.log";
		std::string backupDir = "data/backup/";
		// logging
		int logLevel = 3; // 根据枚举，0 fatal, 1 error, 2 warning, 3 info, 4 debug
		bool logToConsole = true;
		bool logToFile = true;
		// display
		std::string dateFormat = "YYYY-MM-DD";
		std::string sortBy = "name";
		std::string sortOrder = "asc";
		// behavior
		bool autoBackup = true;
		int backupIntervalDays = 7;
		bool confirmOnDelete = true;
		// number
		int last_id = 0;
	};

	/// <summary>加载配置，如果文件不存在则使用默认值并创建</summary>
	inline AppConfig loadConfig(const std::filesystem::path& configPath = "data/config.json") {
		AppConfig cfg;

		auto writeDefaultConfig = [&]() {
			json defaultJson = {
				{"app", {{"name", "家庭信息管理系统"}, {"version", "1.0.0"}}},
				{"paths", {
					{"data_file", cfg.dataFile},
					{"log_file", cfg.logFile},
					{"debug_log_file", cfg.debugLogFile},
					{"backup_dir", cfg.backupDir}
				}},
				{"logging", {
					{"level", 3},
					{"output_to_console", cfg.logToConsole},
					{"output_to_file", cfg.logToFile}
				}},
				{"display", {
					{"date_format", cfg.dateFormat},
					{"sort_by", cfg.sortBy},
					{"sort_order", cfg.sortOrder}
				}},
				{"behavior", {
					{"auto_backup", cfg.autoBackup},
					{"backup_interval_days", cfg.backupIntervalDays},
					{"confirm_on_delete", cfg.confirmOnDelete}
				}},
				{"number", {
					{"last_id", cfg.last_id}
				}}
			};
			if (!configPath.parent_path().empty()) {
				std::filesystem::create_directories(configPath.parent_path());
			}
			std::ofstream file(configPath);
			file << std::setw(4) << defaultJson;
		};

		if (!std::filesystem::exists(configPath)) {
			writeDefaultConfig();
			return cfg; // 返回默认值
		}

		// 文件存在，读取并解析
		std::ifstream file(configPath);
		if (!file.is_open()) {
			std::cerr << "[ERROR] 无法打开配置文件: " << configPath << std::endl;
			writeDefaultConfig();
			return cfg;
		}

		try {
			json j;
			file >> j;

			// 如果json哪个值不存在就使用默认值
			//paths
			cfg.dataFile = j.value("paths", json()).value("data_file", cfg.dataFile);
			cfg.logFile = j.value("paths", json()).value("log_file", cfg.logFile);
			cfg.debugLogFile = j.value("paths", json()).value("debug_log_file", cfg.debugLogFile);
			cfg.backupDir = j.value("paths", json()).value("backup_dir", cfg.backupDir);
			//logging
			auto logObj = j.value("logging", json());
			cfg.logLevel = logObj.value("level", cfg.logLevel);
			cfg.logToConsole = logObj.value("output_to_console", cfg.logToConsole);
			cfg.logToFile = logObj.value("output_to_file", cfg.logToFile);
			//display
			auto dispObj = j.value("display", json());
			cfg.dateFormat = dispObj.value("date_format", cfg.dateFormat);
			cfg.sortBy = dispObj.value("sort_by", cfg.sortBy);
			cfg.sortOrder = dispObj.value("sort_order", cfg.sortOrder);
			// behavior
			auto behObj = j.value("behavior", json());
			cfg.autoBackup = behObj.value("auto_backup", cfg.autoBackup);
			cfg.backupIntervalDays = behObj.value("backup_interval_days", cfg.backupIntervalDays);
			cfg.confirmOnDelete = behObj.value("confirm_on_delete", cfg.confirmOnDelete);
			// number
			auto numObj = j.value("number", json());
			cfg.last_id = numObj.value("last_id", cfg.last_id);
		}
		catch (const json::parse_error& e) {
			std::cerr << "[ERROR] 配置文件 JSON 解析失败: " << e.what() << std::endl;
			writeDefaultConfig();
			return cfg;
		}
		catch (const std::exception& e) {
			std::cerr << "[ERROR] 读取配置文件失败: " << e.what() << std::endl;
			writeDefaultConfig();
			return cfg;
		}

		return cfg;
	}
}