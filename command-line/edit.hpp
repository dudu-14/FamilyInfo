// edit.hpp
// 主要负责文件系统
#pragma once
#include "family.hpp"
#include "log.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

namespace FamilyInfo::Edit
{
	/// <summary>
	/// 写入家庭成员数据到文件
	/// </summary>
	/// <param name="persons">家庭成员列表，std::vector<Person>&</param>
	/// <param name="filePath">文件路径，const std::string&</param>
	/// <returns>无返回值</returns>
	void writePersonData(std::vector<Person>& persons, const std::string& filePath = "data/family_data.json") {
		FamilyInfo::Log::logInfo("正在写入家庭成员数据到文件: " + filePath);
		std::ofstream file(filePath);
		if (!file.is_open()) {
			std::cerr << "[ERROR] 无法打开文件进行写入: " << filePath << std::endl;
			FamilyInfo::Log::logError("无法打开文件进行写入: " + filePath);
			return;
		}
		json j = json::array();
		for (Person& person : persons) {
			j.push_back(person.toJson());
		}
		file << std::setw(4) << j;
		FamilyInfo::Log::logInfo("家庭成员数据已成功写入文件: " + filePath);
	}

	/// <summary>
	/// 从文件加载家庭成员数据，同时恢复原本的ID并更新全局last_id
	/// </summary>
	/// <param name="persons">家庭成员列表，std::vector<Person>&</param>
	/// <param name="filePath">文件路径，const std::string&</param>
	/// <returns>无返回值</returns>
	void loadPersonData(std::vector<Person>& persons, const std::string& filePath = "data/family_data.json") {
		FamilyInfo::Log::logInfo("正在从文件加载家庭成员数据: " + filePath);
		std::ifstream file(filePath);
		if (!file.is_open()) {
			std::cerr << "[ERROR] 无法打开文件进行读取: " << filePath << std::endl;
			FamilyInfo::Log::logError("无法打开文件进行读取: " + filePath);
			return;
		}
		try {
			json j;
			file >> j;
			persons.clear();
			for (const auto& item : j) {
				try {
					std::string name = item.value("name", "");
					std::string birthdayStr = item.value("birthday", "2000-01-01");
					int id = item.value("id", 0);
					std::string sexStr = item.value("sex", "man");
					SexEnum sex = (sexStr == "woman" || sexStr == "女") ? SexEnum::Woman : SexEnum::Man;
					Person p;
					if (id > 0) {
						p = Person(id, name, birthdayStr, sex); // 直接恢复原本的ID，不递增last_id
					}
					else {
						p = Person(name, birthdayStr, sex); // 旧数据没有ID字段，自动分配新ID
					}
					if (p.getId() > last_id) {
						last_id = p.getId(); // 更新全局last_id，避免新增时ID冲突
					}
					persons.push_back(p);
				}
				catch (const std::exception& e) {
					// 单条数据无效（如生日不是真实日期），跳过并警告，不影响其他数据
					std::cerr << "[WARN] 跳过无效的家庭成员数据: " << e.what() << std::endl;
					FamilyInfo::Log::logWarn("跳过无效的家庭成员数据: " + std::string(e.what()));
				}
			}
			FamilyInfo::Log::logInfo("家庭成员数据已成功从文件加载: " + filePath);
		}
		catch (const json::parse_error& e) {
			std::cerr << "[ERROR] 数据文件JSON解析失败: " << e.what() << std::endl;
			FamilyInfo::Log::logError("数据文件JSON解析失败: " + std::string(e.what()));
		}
		catch (const std::exception& e) {
			std::cerr << "[ERROR] 读取数据文件失败: " << e.what() << std::endl;
			FamilyInfo::Log::logError("读取数据文件失败: " + std::string(e.what()));
		}
	}

	/// <summary>
	/// 获取当前时间的字符串，格式为YYYYMMDD_HHMMSS，用于备份文件命名
	/// </summary>
	/// <returns>时间字符串</returns>
	std::string getTimestampString()
	{
		std::time_t t = std::time(nullptr);
		std::tm tm{};
		localtime_s(&tm, &t);
		std::stringstream ss;
		ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
		return ss.str();
	}

	/// <summary>
	/// 备份家庭成员数据文件到备份目录
	/// </summary>
	/// <param name="filePath">数据文件路径，const std::string&</param>
	/// <param name="backupDir">备份目录路径，const std::string&</param>
	/// <returns>成功返回true，失败返回false</returns>
	bool backupPersonData(const std::string& filePath, const std::string& backupDir) {
		FamilyInfo::Log::logInfo("正在备份数据文件: " + filePath);
		if (!std::filesystem::exists(filePath)) {
			std::cerr << "[ERROR] 数据文件不存在，无法备份: " << filePath << std::endl;
			FamilyInfo::Log::logError("数据文件不存在，无法备份: " + filePath);
			return false;
		}
		try {
			std::filesystem::create_directories(backupDir);
			std::filesystem::path backupPath = std::filesystem::path(backupDir) / ("family_data_" + getTimestampString() + ".json");
			std::filesystem::copy_file(filePath, backupPath, std::filesystem::copy_options::overwrite_existing);
			FamilyInfo::Log::logInfo("备份成功: " + backupPath.string());
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ERROR] 备份失败: " << e.what() << std::endl;
			FamilyInfo::Log::logError("备份失败: " + std::string(e.what()));
			return false;
		}
	}

	/// <summary>
	/// 列出备份目录中的所有备份文件
	/// </summary>
	/// <param name="backupDir">备份目录路径，const std::string&</param>
	/// <returns>备份文件路径列表，std::vector&lt;std::string&gt;</returns>
	std::vector<std::string> listBackups(const std::string& backupDir) {
		std::vector<std::string> backups;
		if (!std::filesystem::exists(backupDir)) {
			return backups;
		}
		for (const auto& entry : std::filesystem::directory_iterator(backupDir)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				backups.push_back(entry.path().string());
			}
		}
		std::sort(backups.begin(), backups.end()); // 按文件名排序，时间戳文件名即为时间顺序
		return backups;
	}

	/// <summary>
	/// 从备份文件恢复数据到数据文件
	/// </summary>
	/// <param name="dataFile">数据文件路径，const std::string&</param>
	/// <param name="backupFile">备份文件路径，const std::string&</param>
	/// <returns>成功返回true，失败返回false</returns>
	bool restorePersonData(const std::string& dataFile, const std::string& backupFile) {
		FamilyInfo::Log::logInfo("正在从备份恢复数据: " + backupFile);
		if (!std::filesystem::exists(backupFile)) {
			std::cerr << "[ERROR] 备份文件不存在: " << backupFile << std::endl;
			FamilyInfo::Log::logError("备份文件不存在: " + backupFile);
			return false;
		}
		try {
			std::filesystem::copy_file(backupFile, dataFile, std::filesystem::copy_options::overwrite_existing);
			FamilyInfo::Log::logInfo("数据恢复成功: " + dataFile);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ERROR] 恢复失败: " << e.what() << std::endl;
			FamilyInfo::Log::logError("恢复失败: " + std::string(e.what()));
			return false;
		}
	}

	/// <summary>
	/// 导出家庭成员数据到CSV文件
	/// </summary>
	/// <param name="persons">家庭成员列表，const std::vector&lt;Person&gt;&amp;</param>
	/// <param name="filePath">导出文件路径，const std::string&amp;</param>
	/// <returns>成功返回true，失败返回false</returns>
	/// <remarks>
	/// 文件带UTF-8 BOM并使用CRLF换行，方便Excel直接打开识别中文。
	/// </remarks>
	bool exportPersonData(const std::vector<Person>& persons, const std::string& filePath = "data/export.csv") {
		FamilyInfo::Log::logInfo("正在导出家庭成员数据到CSV文件: " + filePath);
		try {
			if (!std::filesystem::path(filePath).parent_path().empty()) {
				std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
			}
			std::ofstream file(filePath, std::ios::binary); // 二进制模式，避免换行符转换影响BOM
			if (!file.is_open()) {
				std::cerr << "[ERROR] 无法打开文件进行导出: " << filePath << std::endl;
				FamilyInfo::Log::logError("无法打开文件进行导出: " + filePath);
				return false;
			}
			file << "\xEF\xBB\xBF"; // UTF-8 BOM
			file << "ID,姓名,生日,性别,年龄\r\n";
			for (const auto& person : persons) {
				file << person.getId() << "," << person.getName() << "," << person.getBirthdayString()
					<< "," << person.getSex() << "," << person.getAge() << "\r\n";
			}
			FamilyInfo::Log::logInfo("家庭成员数据已成功导出: " + filePath);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ERROR] 导出失败: " << e.what() << std::endl;
			FamilyInfo::Log::logError("导出失败: " + std::string(e.what()));
			return false;
		}
	}

	/// <summary>
	/// 检查是否需要自动备份
	/// </summary>
	/// <param name="backupDir">备份目录路径，const std::string&</param>
	/// <param name="intervalDays">备份间隔天数，int</param>
	/// <returns>需要备份返回true，否则返回false</returns>
	bool needAutoBackup(const std::string& backupDir, int intervalDays) {
		if (intervalDays <= 0) {
			return true; // 间隔小于等于0表示每次启动都备份
		}
		// 找到最新的备份文件时间
		std::filesystem::file_time_type newestTime;
		bool found = false;
		if (std::filesystem::exists(backupDir)) {
			for (const auto& entry : std::filesystem::directory_iterator(backupDir)) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					auto fileTime = entry.last_write_time();
					if (!found || fileTime > newestTime) {
						newestTime = fileTime;
						found = true;
					}
				}
			}
		}
		if (!found) {
			return true; // 没有备份文件，需要备份
		}
		// 判断最新备份是否已经超过间隔时间
		auto now = std::filesystem::file_time_type::clock::now();
		auto elapsed = now - newestTime;
		return elapsed > std::chrono::hours(24) * intervalDays;
	}
}
