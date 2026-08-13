// edit.hpp
// 主要负责文件系统
#pragma once
#include "family.hpp"
#include <fstream>
#include <iostream>
#include "json.hpp"
#include <math.h>
#include "log.hpp"

using json = nlohmann::json;

namespace FamilyInfo::Edit
{
	/// <summary>
	/// 写入家庭成员数据到文件
	/// </summary>
	/// <param name="persons">家庭成员列表，std::vector<Person>&</param>
	/// <param name="filePath">文件路径，const std::string&</param>
	/// <returns></returns>
	void writePersonData(std::vector<Person>& persons, const std::string& filePath = "data/family_data.json") {
		FamilyInfo::Log::logInfo("正在写入家庭成员数据到文件: " + filePath);
		std::ofstream file(filePath);
		if (!file.is_open()) {
			std::cerr << "[ERROR] 无法打开文件进行写入: " << filePath << std::endl;
			FamilyInfo::Log::logError("无法打开文件进行写入: " + filePath);
		}
		json j = json::array();
		for (Person& person : persons) {
			j.push_back(person.toJson());
		}
		file << std::setw(4) << j;
		FamilyInfo::Log::logInfo("家庭成员数据已成功写入文件: " + filePath);
	}

	void loadPersonData(std::vector<Person>& persons, const std::string& filePath = "data/family_data.json") {
		FamilyInfo::Log::logInfo("正在从文件加载家庭成员数据: " + filePath);
		std::ifstream file(filePath);
		if (!file.is_open()) {
			std::cerr << "[ERROR] 无法打开文件进行读取: " << filePath << std::endl;
			FamilyInfo::Log::logError("无法打开文件进行读取: " + filePath);
			return;
		}
		json j;
		file >> j;
		persons.clear();
		for (const auto& item : j) {
			std::string name = item["name"];
			std::string birthdayStr = item["birthday"];
			int birthdayInt = std::stoi(birthdayStr.substr(0, 4) + birthdayStr.substr(5, 2) + birthdayStr.substr(8, 2));
			SexEnum sex = item["sex"] == "man" ? SexEnum::Man : SexEnum::Woman;
			Person p(name, birthdayInt, sex);
			persons.push_back(p);
		}
		FamilyInfo::Log::logInfo("家庭成员数据已成功从文件加载: " + filePath);
	}
}