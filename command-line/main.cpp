// main.cpp
#include "config.hpp"
#include "family.hpp"
#include "log.hpp"
#include "edit.hpp"
#include <iostream>
#include <string>

using namespace FamilyInfo;
Config::AppConfig g_config; // 全局配置对象
std::vector<Person> g_familyMembers; // 全局家庭成员列表

void pause()
{
	std::cout << "按回车键继续..." << std::endl;
	std::cin.get();
}

void readFamilyData();

/// <summary>
/// 程序主入口
/// </summary>
/// <returns>退出时返回0</returns>
int main()
{
	using namespace std;

	g_config = Config::loadConfig();
	Log::initLog(g_config.logFile, g_config.debugLogFile, Log::configToLogLevel(g_config.logLevel));
	last_id = g_config.last_id; // 初始化全局last_id
	cout << "======家庭信息管理系统 v1.0.0=====\n";
	while (1)
	{
		// 主循环

		Edit::loadPersonData(g_familyMembers, g_config.dataFile); // 每次循环都加载数据，确保数据最新

		Log::logInfo("进入主循环，当前家庭成员数量: " + std::to_string(g_familyMembers.size()));
		cout << "1. 添加家庭成员\n";
		cout << "2. 显示家庭成员\n";
		cout << "3. 删除家庭成员\n";
		cout << "4. 编辑家庭成员\n";
		cout << "5. 备份数据\n";
		cout << "6. 恢复数据\n";
		cout << "7. 设置\n";
		cout << "0. 退出\n";
		cout << "请选择操作: ";
		int s;
		scanf("%d", &s);

		if (!s) 
		{
			Edit::writePersonData(g_familyMembers, g_config.dataFile); // 保存数据
			//退出程序
			cout << "你确认要退出吗？(y/n): ";
			char c;
			scanf("%c", &c);
			if (c == 'n' || c == 'N')
			{
				continue;
			}
			exit(0);
		}
		if (s == 1)
		{
			// 添加家庭成员
			cout << "添加家庭成员\n请输入姓名: ";
			string name;
			cin >> name;
			cout << "请输入生日 (YYYY-MM-DD): ";
			string birthday;
			cin >> birthday;
			cout << "请输入性别 (0-男, 1-女): ";
			int sexInt;
			cin >> sexInt;
			Person p(name, birthday, SexEnum(sexInt));
			g_familyMembers.push_back(p);
			cout << "家庭成员添加成功: " << name << endl;
			Edit::writePersonData(g_familyMembers, g_config.dataFile);
			continue;
		}
		if (s == 2)
		{
			// 显示家庭成员
			cout << "======家庭成员列表======\n";
			if (g_familyMembers.empty())
			{
				cout << "没有家庭成员数据。\n";
				continue;
			}
			for (auto& person : g_familyMembers)
			{
				cout << "ID: " << person.getId()
					<< ", 姓名: " << person.getName()
					<< ", 生日: " << person.getBirthdayString()
					<< ", 年龄: " << person.getAge()
					<< ", 性别: " << person.getSex()
					<< endl;
			}
		}
	}
	return 0;
}