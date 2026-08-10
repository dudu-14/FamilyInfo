// main.cpp
#include "config.hpp"
#include "family.hpp"
#include "log.hpp"

using namespace FamilyInfo;
Config::AppConfig g_config; // 全局配置对象

void pause()
{
	std::cout << "按回车键继续..." << std::endl;
	std::cin.get();
}


/// <summary>
/// 程序主入口
/// </summary>
/// <returns>退出时返回0</returns>
int main()
{
	using namespace std;

	g_config = Config::loadConfig();
	Log::initLog(g_config.logFile, g_config.debugLogFile, Log::configToLogLevel(g_config.logLevel));
	cout << "======家庭信息管理系统 v1.0.0=====\n";

	FamilyInfo::exit(0);
}