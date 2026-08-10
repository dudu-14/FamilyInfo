// config.hpp
#pragma once
#include <filesystem>
#include <iostream>
#include "family.hpp"
#include "json.hpp"

namespace FamilyInfo
{
	/// <summary>
	/// 检测配置文件是否存在
	/// </summary>
	/// <returns>bool类型，存在返回true，否则返回false</returns>
	inline bool checkFileExists(std::filesystem::path _configPath = "config.ini")
	{
		return std::filesystem::exists(_configPath);
	}

	
}