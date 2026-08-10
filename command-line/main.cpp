// main.cpp
#include "config.hpp"
#include "family.hpp"
#include "log.hpp"

using namespace FamilyInfo;
Config::AppConfig g_config; // 全局配置对象

/// <summary>
/// 初始化整体
/// </summary>
/// <returns>0为正常，非0为异常</returns>
bool init()
{
	using namespace FamilyInfo::Log;
	if (!FamilyInfo::Config::init()) {
		logWarn("无法初始化配置文件，配置存储可能失效");
		logDebug("配置文件初始化失败");
		return true;
	}
	logInfo("配置文件初始化成功");

	return false;
}

/// <summary>
/// 程序主入口
/// </summary>
/// <returns>退出时返回0</returns>
int main()
{
	using namespace std;
	Log::initLog();

	if (init())
	{
		cerr << "[ERROR]程序初始化失败，具体内容可查看日志文件，目录为logs/";
	}

}