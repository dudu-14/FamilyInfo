// ui.hpp
// 负责控制台界面的美化（Windows彩色输出）
#pragma once
#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // 禁止windows.h定义min/max宏，避免与std::numeric_limits冲突
#endif
#include <windows.h>
#endif

namespace FamilyInfo::UI
{
	/// <summary>
	/// 设置控制台文字颜色
	/// </summary>
	/// <param name="color">颜色值，0-15，见Windows控制台颜色定义</param>
	inline void setColor(int color)
	{
#ifdef _WIN32
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#endif
	}

	/// <summary>
	/// 恢复默认颜色（浅灰色）
	/// </summary>
	inline void resetColor()
	{
		setColor(7);
	}

	/// <summary>
	/// 输出标题（青色）
	/// </summary>
	/// <param name="text">标题文本</param>
	inline void printTitle(const std::string& text)
	{
		setColor(11);
		std::cout << text;
		resetColor();
	}

	/// <summary>
	/// 输出成功信息（绿色）
	/// </summary>
	/// <param name="text">成功文本</param>
	inline void printSuccess(const std::string& text)
	{
		setColor(10);
		std::cout << text;
		resetColor();
	}

	/// <summary>
	/// 输出错误信息（红色）
	/// </summary>
	/// <param name="text">错误文本</param>
	inline void printError(const std::string& text)
	{
		setColor(12);
		std::cout << text;
		resetColor();
	}

	/// <summary>
	/// 输出警告信息（黄色）
	/// </summary>
	/// <param name="text">警告文本</param>
	inline void printWarn(const std::string& text)
	{
		setColor(14);
		std::cout << text;
		resetColor();
	}
}
