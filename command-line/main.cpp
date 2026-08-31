// main.cpp
#define WIN32_LEAN_AND_MEAN // 在包含windows.h前定义，避免winsock.h与winsock2.h冲突
#include "config.hpp"
#include "family.hpp"
#include "log.hpp"
#include "edit.hpp"
#include "ui.hpp"
#include "server.hpp"
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace FamilyInfo;
Config::AppConfig g_config; // 全局配置对象
std::vector<Person> g_familyMembers; // 全局家庭成员列表

void pause()
{
	std::cout << "按回车键继续..." << std::endl;
	std::cin.get();
}

/// <summary>
/// 清空输入缓冲区，避免残留的换行符或非法输入影响后续读取
/// </summary>
void clearInput()
{
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

/// <summary>
/// 根据ID获取家庭成员姓名
/// </summary>
/// <param name="id">成员ID</param>
/// <returns>姓名，找不到返回"未知"</returns>
std::string getPersonName(int id)
{
	for (const auto& person : g_familyMembers)
	{
		if (person.getId() == id)
		{
			return person.getName();
		}
	}
	return "未知";
}

/// <summary>
/// 打开默认浏览器访问网页管理系统
/// </summary>
/// <param name="port">后端端口</param>
void openWebPage(int port)
{
	std::string url = "http://127.0.0.1:" + std::to_string(port) + "/";
	HINSTANCE result = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	if ((INT_PTR)result <= 32)
	{
		UI::printError("打开浏览器失败，请手动在浏览器访问: " + url + "\n");
	}
	else
	{
		UI::printSuccess("已打开网页管理系统: " + url + "\n");
		UI::printWarn("提示：如果页面打不开，请先另开终端运行 command-line.exe --server " + std::to_string(port) + " 启动后端。\n");
	}
}

/// <summary>
/// 程序主入口
/// </summary>
/// <param name="argc">参数个数</param>
/// <param name="argv">参数列表</param>
/// <returns>退出时返回0</returns>
int main(int argc, char* argv[])
{
	// 服务器模式：command-line.exe --server [端口]，供GUI等外部程序通过REST API接入
	if (argc >= 2 && std::string(argv[1]) == "--server")
	{
		int port = 8080;
		if (argc >= 3)
		{
			port = std::atoi(argv[2]);
		}
		g_config = Config::loadConfig();
		Log::initLog(g_config.logFile, g_config.debugLogFile, Log::configToLogLevel(g_config.logLevel));
		last_id = g_config.last_id; // 初始化全局last_id
		return Server::runServer(port);
	}

	using namespace std;

	g_config = Config::loadConfig();
	Log::initLog(g_config.logFile, g_config.debugLogFile, Log::configToLogLevel(g_config.logLevel));
	last_id = g_config.last_id; // 初始化全局last_id
	UI::printTitle("======家庭信息管理系统 v1.0.0=====\n");
	while (1)
	{
		// 主循环

		Edit::loadPersonData(g_familyMembers, g_config.dataFile); // 每次循环都加载数据，确保数据最新
		g_config.last_id = last_id; // 同步配置中的last_id

		// 自动备份检查
		if (g_config.autoBackup && std::filesystem::exists(g_config.dataFile)
			&& Edit::needAutoBackup(g_config.backupDir, g_config.backupIntervalDays))
		{
			Edit::backupPersonData(g_config.dataFile, g_config.backupDir);
		}

		Log::logInfo("进入主循环，当前家庭成员数量: " + std::to_string(g_familyMembers.size()));
		cout << "1. 添加家庭成员\n";
		cout << "2. 显示家庭成员\n";
		cout << "3. 删除家庭成员\n";
		cout << "4. 编辑家庭成员\n";
		cout << "5. 备份数据\n";
		cout << "6. 恢复数据\n";
		cout << "7. 设置\n";
		cout << "8. 搜索家庭成员\n";
		cout << "9. 统计信息\n";
		cout << "10. 导出CSV\n";
		cout << "11. 日志管理\n";
		cout << "12. 亲属关系\n";
		cout << "13. 打开网页\n";
		cout << "0. 退出\n";
		cout << "请选择操作: ";
		int s;
		if (scanf("%d", &s) != 1) // 输入的不是数字，清空缓冲区后重新输入
		{
			int c;
			while ((c = getchar()) != '\n' && c != EOF); // 清空缓冲区
			if (c == EOF) break; // 输入结束（如管道重定向），退出程序
			cout << "输入无效，请输入数字。\n";
			continue;
		}

		if (!s)
		{
			// 退出程序
			cout << "你确认要退出吗？(y/n): ";
			char c;
			int gc;
			while ((gc = getchar()) != '\n' && gc != EOF); // 清空缓冲区，防止读到上次输入留下的换行符
			if (gc == EOF) break; // 输入结束，直接退出
			c = getchar();
			if (c == 'n' || c == 'N')
			{
				continue;
			}
			Config::saveConfig(g_config); // 保存配置（含last_id）
			Edit::writePersonData(g_familyMembers, g_config.dataFile); // 保存数据
			break;
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
			if (!(cin >> sexInt))
			{
				clearInput();
				UI::printWarn("性别输入无效，添加失败。\n");
				continue;
			}
			if (sexInt != 0 && sexInt != 1)
			{
				UI::printWarn("性别输入无效，添加失败。\n");
				continue;
			}
			try
			{
				Person p(name, birthday, SexEnum(sexInt));
				g_familyMembers.push_back(p);
				UI::printSuccess("家庭成员添加成功: " + name + "\n");
				Edit::writePersonData(g_familyMembers, g_config.dataFile);
				g_config.last_id = last_id; // 更新配置中的last_id
				Config::saveConfig(g_config);
			}
			catch (const std::exception& e)
			{
				UI::printError("添加失败: " + std::string(e.what()) + "\n");
			}
			continue;
		}
		if (s == 2)
		{
			// 显示家庭成员
			UI::printTitle("======家庭成员列表======\n");
			if (g_familyMembers.empty())
			{
				cout << "没有家庭成员数据。\n";
				continue;
			}
			std::vector<Person> displayList = g_familyMembers; // 拷贝一份用于排序，不影响原数据
			// 根据配置排序
			if (g_config.sortBy == "id")
			{
				std::sort(displayList.begin(), displayList.end(),
					[](const Person& a, const Person& b) { return a.getId() < b.getId(); });
			}
			else if (g_config.sortBy == "age")
			{
				std::sort(displayList.begin(), displayList.end(),
					[](const Person& a, const Person& b) { return a.getAge() < b.getAge(); });
			}
			else if (g_config.sortBy == "birthday")
			{
				// 生日字符串为YYYY-MM-DD格式，字典序就是时间顺序
				std::sort(displayList.begin(), displayList.end(),
					[](const Person& a, const Person& b) { return a.getBirthdayString() < b.getBirthdayString(); });
			}
			else
			{
				std::sort(displayList.begin(), displayList.end(),
					[](const Person& a, const Person& b) { return a.getName() < b.getName(); });
			}
			if (g_config.sortOrder == "desc")
			{
				std::reverse(displayList.begin(), displayList.end());
			}
			for (auto& person : displayList)
			{
				cout << "ID: " << person.getId()
					<< ", 姓名: " << person.getName()
					<< ", 生日: " << person.getBirthdayString()
					<< ", 年龄: " << person.getAge()
					<< ", 性别: " << person.getSex()
					<< endl;
			}
			continue;
		}
		if (s == 3)
		{
			// 删除家庭成员
			cout << "请输入要删除的家庭成员ID: ";
			int delId;
			if (!(cin >> delId))
			{
				clearInput();
				cout << "ID输入无效。\n";
				continue;
			}
			auto it = std::find_if(g_familyMembers.begin(), g_familyMembers.end(),
				[delId](const Person& p) { return p.getId() == delId; });
			if (it == g_familyMembers.end())
			{
				UI::printWarn("未找到ID为 " + std::to_string(delId) + " 的家庭成员。\n");
				continue;
			}
			cout << "将删除: ID: " << it->getId() << ", 姓名: " << it->getName()
				<< ", 生日: " << it->getBirthdayString() << endl;
			if (g_config.confirmOnDelete) // 根据配置决定是否需要确认
			{
				cout << "你确认要删除吗？(y/n): ";
				char c;
				cin >> c;
				if (c == 'n' || c == 'N')
				{
					cout << "已取消删除。\n";
					continue;
				}
			}
			g_familyMembers.erase(it);
			// 清理其他人身上指向该成员的关系，避免留下无效引用
			for (auto& person : g_familyMembers)
			{
				std::vector<Relation> rels = person.getRelations(); // 取副本，遍历时修改原列表是安全的
				for (const auto& rel : rels)
				{
					if (rel.targetId == delId)
					{
						person.removeRelation(rel.type, delId);
					}
				}
			}
			UI::printSuccess("家庭成员删除成功: " + std::to_string(delId) + "\n");
			Edit::writePersonData(g_familyMembers, g_config.dataFile);
			continue;
		}
		if (s == 4)
		{
			// 编辑家庭成员
			cout << "请输入要编辑的家庭成员ID: ";
			int editId;
			if (!(cin >> editId))
			{
				clearInput();
				cout << "ID输入无效。\n";
				continue;
			}
			auto it = std::find_if(g_familyMembers.begin(), g_familyMembers.end(),
				[editId](const Person& p) { return p.getId() == editId; });
			if (it == g_familyMembers.end())
			{
				UI::printWarn("未找到ID为 " + std::to_string(editId) + " 的家庭成员。\n");
				continue;
			}
			while (1)
			{
				// 编辑子菜单
				UI::printTitle("======编辑家庭成员======\n");
				cout << "当前信息: 姓名: " << it->getName() << ", 生日: " << it->getBirthdayString()
					<< ", 性别: " << it->getSex() << endl;
				cout << "1. 修改姓名\n";
				cout << "2. 修改生日\n";
				cout << "3. 修改性别\n";
				cout << "0. 返回\n";
				cout << "请选择操作: ";
				int opt;
				if (!(cin >> opt))
				{
					clearInput();
					if (std::cin.eof()) break; // 输入结束（如管道重定向），退出子菜单
					cout << "输入无效，请输入数字。\n";
					continue;
				}
				if (!opt)
				{
					break;
				}
				if (opt == 1)
				{
					// 修改姓名
					cout << "请输入新的姓名: ";
					string newName;
					cin >> newName;
					it->setName(newName);
					UI::printSuccess("姓名修改成功。\n");
				}
				else if (opt == 2)
				{
					// 修改生日
					cout << "请输入新的生日 (YYYY-MM-DD): ";
					string newBirthday;
					cin >> newBirthday;
					try
					{
						it->setBirthday(newBirthday);
						UI::printSuccess("生日修改成功。\n");
					}
					catch (const std::exception& e)
					{
						cout << "生日修改失败: " << e.what() << endl;
					}
				}
				else if (opt == 3)
				{
					// 修改性别
					cout << "请输入新的性别 (0-男, 1-女): ";
					int newSex;
					if (!(cin >> newSex))
					{
						clearInput();
						cout << "性别输入无效。\n";
						continue;
					}
					if (newSex != 0 && newSex != 1)
					{
						cout << "性别输入无效。\n";
					}
					else
					{
						it->setSex(SexEnum(newSex));
						UI::printSuccess("性别修改成功。\n");
					}
				}
			}
			Edit::writePersonData(g_familyMembers, g_config.dataFile);
			UI::printSuccess("家庭成员信息已保存。\n");
			continue;
		}
		if (s == 5)
		{
			// 备份数据
			if (Edit::backupPersonData(g_config.dataFile, g_config.backupDir))
			{
				UI::printSuccess("备份成功，备份目录: " + g_config.backupDir + "\n");
			}
			else
			{
				UI::printError("备份失败。\n");
			}
			continue;
		}
		if (s == 6)
		{
			// 恢复数据
			std::vector<std::string> backups = Edit::listBackups(g_config.backupDir);
			if (backups.empty())
			{
				cout << "没有找到任何备份文件。\n";
				continue;
			}
			UI::printTitle("======备份列表======\n");
			for (size_t i = 0; i < backups.size(); i++)
			{
				cout << i + 1 << ". " << backups[i] << endl;
			}
			cout << "请选择要恢复的备份 (0-返回): ";
			int choice;
			if (!(cin >> choice))
			{
				clearInput();
				cout << "选择无效。\n";
				continue;
			}
			if (choice <= 0 || choice > (int)backups.size())
			{
				continue;
			}
			cout << "恢复后当前数据将被覆盖，你确认吗？(y/n): ";
			char c;
			cin >> c;
			if (c == 'n' || c == 'N')
			{
				cout << "已取消恢复。\n";
				continue;
			}
			if (Edit::restorePersonData(g_config.dataFile, backups[choice - 1]))
			{
				Edit::loadPersonData(g_familyMembers, g_config.dataFile); // 重新加载数据
				UI::printSuccess("数据恢复成功。\n");
			}
			else
			{
				UI::printError("数据恢复失败。\n");
			}
			continue;
		}
		if (s == 7)
		{
			// 设置
			while (1)
			{
				// 设置子菜单
				UI::printTitle("======设置======\n");
				cout << "1. 删除前确认 (当前: " << (g_config.confirmOnDelete ? "开" : "关") << ")\n";
				cout << "2. 自动备份 (当前: " << (g_config.autoBackup ? "开" : "关") << ")\n";
				cout << "3. 控制台日志输出 (当前: " << (g_config.logToConsole ? "开" : "关") << ")\n";
				cout << "4. 日志级别 (当前: " << g_config.logLevel << ")\n";
				cout << "5. 修改排序字段 (当前: " << g_config.sortBy << ")\n";
				cout << "6. 修改排序顺序 (当前: " << g_config.sortOrder << ")\n";
				cout << "7. 保存设置并返回\n";
				cout << "0. 返回\n";
				cout << "请选择操作: ";
				int opt;
				if (!(cin >> opt))
				{
					clearInput();
					if (std::cin.eof()) break; // 输入结束（如管道重定向），退出子菜单
					cout << "输入无效，请输入数字。\n";
					continue;
				}
				if (!opt)
				{
					break;
				}
				if (opt == 1)
				{
					// 切换删除前确认
					g_config.confirmOnDelete = !g_config.confirmOnDelete;
					cout << "删除前确认已" << (g_config.confirmOnDelete ? "开启" : "关闭") << "。\n";
				}
				else if (opt == 2)
				{
					// 切换自动备份
					g_config.autoBackup = !g_config.autoBackup;
					cout << "自动备份已" << (g_config.autoBackup ? "开启" : "关闭") << "。\n";
				}
				else if (opt == 3)
				{
					// 切换控制台日志输出
					g_config.logToConsole = !g_config.logToConsole;
					cout << "控制台日志输出已" << (g_config.logToConsole ? "开启" : "关闭") << "。\n";
				}
				else if (opt == 4)
				{
					// 修改日志级别
					cout << "请输入新的日志级别 (0-FATAL, 1-ERROR, 2-WARNING, 3-INFO, 4-DEBUG): ";
					int newLevel;
					if (!(cin >> newLevel))
					{
						clearInput();
						cout << "日志级别输入无效。\n";
						continue;
					}
					if (newLevel < 0 || newLevel > 4)
					{
						cout << "日志级别无效。\n";
					}
					else
					{
						g_config.logLevel = newLevel;
						Log::setLogLevel(Log::configToLogLevel(newLevel));
						UI::printSuccess("日志级别修改成功。\n");
					}
				}
				else if (opt == 5)
				{
					// 修改排序字段
					cout << "请选择排序字段 (1-姓名, 2-ID, 3-年龄, 4-生日): ";
					int field;
					if (!(cin >> field))
					{
						clearInput();
						cout << "输入无效。\n";
						continue;
					}
					if (field == 2) g_config.sortBy = "id";
					else if (field == 3) g_config.sortBy = "age";
					else if (field == 4) g_config.sortBy = "birthday";
					else g_config.sortBy = "name";
					cout << "排序字段已修改为: " << g_config.sortBy << endl;
				}
				else if (opt == 6)
				{
					// 修改排序顺序
					cout << "请选择排序顺序 (1-升序, 2-降序): ";
					int order;
					if (!(cin >> order))
					{
						clearInput();
						cout << "输入无效。\n";
						continue;
					}
					g_config.sortOrder = (order == 2) ? "desc" : "asc";
					cout << "排序顺序已修改为: " << g_config.sortOrder << endl;
				}
				else if (opt == 7)
				{
					// 保存设置
					Config::saveConfig(g_config);
					UI::printSuccess("设置已保存。\n");
					break;
				}
			}
			continue;
		}
		if (s == 8)
		{
			// 搜索家庭成员
			UI::printTitle("======搜索家庭成员======\n");
			cout << "1. 按姓名搜索\n";
			cout << "2. 按ID搜索\n";
			cout << "请选择搜索方式: ";
			int searchType;
			if (!(cin >> searchType))
			{
				clearInput();
				cout << "输入无效。\n";
				continue;
			}
			if (searchType == 1)
			{
				// 按姓名搜索（包含关键字即匹配，不区分大小写）
				cout << "请输入姓名关键字: ";
				string keyword;
				cin >> keyword;
				// 统一转小写，实现不区分大小写的匹配
				std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
				bool found = false;
				for (auto& person : g_familyMembers)
				{
					std::string name = person.getName();
					std::transform(name.begin(), name.end(), name.begin(), ::tolower);
					if (name.find(keyword) != std::string::npos)
					{
						cout << "ID: " << person.getId()
							<< ", 姓名: " << person.getName()
							<< ", 生日: " << person.getBirthdayString()
							<< ", 年龄: " << person.getAge()
							<< ", 性别: " << person.getSex()
							<< endl;
						found = true;
					}
				}
				if (!found)
				{
					UI::printWarn("未找到姓名包含\"" + keyword + "\"的家庭成员。\n");
				}
			}
			else if (searchType == 2)
			{
				// 按ID搜索
				cout << "请输入ID: ";
				int searchId;
				if (!(cin >> searchId))
				{
					clearInput();
					cout << "ID输入无效。\n";
					continue;
				}
				auto it = std::find_if(g_familyMembers.begin(), g_familyMembers.end(),
					[searchId](const Person& p) { return p.getId() == searchId; });
				if (it == g_familyMembers.end())
				{
					UI::printWarn("未找到ID为 " + std::to_string(searchId) + " 的家庭成员。\n");
				}
				else
				{
					cout << "ID: " << it->getId()
						<< ", 姓名: " << it->getName()
						<< ", 生日: " << it->getBirthdayString()
						<< ", 年龄: " << it->getAge()
						<< ", 性别: " << it->getSex()
						<< endl;
				}
			}
			else
			{
				cout << "搜索方式无效。\n";
			}
			continue;
		}
		if (s == 9)
		{
			// 统计信息
			UI::printTitle("======统计信息======\n");
			if (g_familyMembers.empty())
			{
				cout << "没有家庭成员数据。\n";
				continue;
			}
			int manCount = 0;
			int womanCount = 0;
			int ageSum = 0;
			int oldestAge = -1;
			int youngestAge = 999;
			std::string oldestName;
			std::string youngestName;
			for (auto& person : g_familyMembers)
			{
				int age = person.getAge();
				if (age < 0) age = 0; // 生日在未来等异常情况按0处理
				ageSum += age;
				if (person.getSex() == "男") {
					manCount++;
				}
				else {
					womanCount++;
				}
				if (oldestAge < 0 || age > oldestAge) {
					oldestAge = age;
					oldestName = person.getName();
				}
				if (age < youngestAge) {
					youngestAge = age;
					youngestName = person.getName();
				}
			}
			double avgAge = (double)ageSum / g_familyMembers.size();
			cout << "家庭成员总数: " << g_familyMembers.size() << endl;
			cout << "男性人数: " << manCount << ", 女性人数: " << womanCount << endl;
			cout << "平均年龄: " << std::fixed << std::setprecision(1) << avgAge << endl;
			cout << "最年长: " << oldestName << " (" << oldestAge << "岁)" << endl;
			cout << "最年幼: " << youngestName << " (" << youngestAge << "岁)" << endl;
			continue;
		}
		if (s == 10)
		{
			// 导出CSV
			if (Edit::exportPersonData(g_familyMembers, "data/export.csv"))
			{
				UI::printSuccess("导出成功，文件: data/export.csv\n");
			}
			else
			{
				UI::printError("导出失败。\n");
			}
			continue;
		}
		if (s == 11)
		{
			// 日志管理
			while (1)
			{
				UI::printTitle("======日志管理======\n");
				cout << "1. 查看日志（最近30条）\n";
				cout << "2. 清空日志文件\n";
				cout << "0. 返回\n";
				cout << "请选择操作: ";
				int opt;
				if (!(cin >> opt))
				{
					clearInput();
					if (std::cin.eof()) break; // 输入结束（如管道重定向），退出子菜单
					cout << "输入无效，请输入数字。\n";
					continue;
				}
				if (!opt)
				{
					break;
				}
				if (opt == 1)
				{
					// 查看日志
					std::vector<std::string> lines = Log::readLogLines(g_config.logFile, 30);
					if (lines.empty())
					{
						cout << "日志文件为空或不存在。\n";
					}
					else
					{
						UI::printTitle("======最近日志======\n");
						for (const auto& line : lines)
						{
							cout << line << endl;
						}
					}
				}
				else if (opt == 2)
				{
					// 清空日志
					if (Log::clearLogFile(g_config.logFile))
					{
						cout << "日志文件已清空。\n";
					}
					else
					{
						cout << "清空日志失败。\n";
					}
				}
			}
			continue;
		}
		if (s == 12)
		{
			// 亲属关系
			while (1)
			{
				UI::printTitle("======亲属关系======\n");
				cout << "1. 添加关系\n";
				cout << "2. 删除关系\n";
				cout << "3. 查看关系\n";
				cout << "0. 返回\n";
				cout << "请选择操作: ";
				int opt;
				if (!(cin >> opt))
				{
					clearInput();
					if (std::cin.eof()) break; // 输入结束（如管道重定向），退出子菜单
					cout << "输入无效，请输入数字。\n";
					continue;
				}
				if (!opt)
				{
					break;
				}
				if (opt == 1)
				{
					// 添加关系
					cout << "请输入成员A的ID: ";
					int idA;
					if (!(cin >> idA))
					{
						clearInput();
						cout << "ID输入无效。\n";
						continue;
					}
					auto itA = std::find_if(g_familyMembers.begin(), g_familyMembers.end(),
						[idA](const Person& p) { return p.getId() == idA; });
					if (itA == g_familyMembers.end())
					{
						UI::printWarn("未找到ID为 " + std::to_string(idA) + " 的家庭成员。\n");
						continue;
					}
					cout << "请选择关系类型 (1-父亲, 2-母亲, 3-配偶, 4-子女, 5-兄弟姐妹): ";
					int typeInt;
					if (!(cin >> typeInt))
					{
						clearInput();
						cout << "关系类型输入无效。\n";
						continue;
					}
					if (typeInt < 1 || typeInt > 5)
					{
						UI::printWarn("关系类型无效。\n");
						continue;
					}
					RelationType type = (RelationType)typeInt; // 菜单1-5正好对应Father..Sibling
					cout << "请输入成员B的ID: ";
					int idB;
					if (!(cin >> idB))
					{
						clearInput();
						cout << "ID输入无效。\n";
						continue;
					}
					auto itB = std::find_if(g_familyMembers.begin(), g_familyMembers.end(),
						[idB](const Person& p) { return p.getId() == idB; });
					if (itB == g_familyMembers.end())
					{
						UI::printWarn("未找到ID为 " + std::to_string(idB) + " 的家庭成员。\n");
						continue;
					}
					if (idA == idB)
					{
						UI::printWarn("不能和自己建立关系。\n");
						continue;
					}
					if (itA->hasRelation(type, idB))
					{
						UI::printWarn("该关系已存在。\n");
						continue;
					}
					itA->addRelation(type, idB);
					itB->addRelation(getReverseRelation(type), idA); // 自动补全反向关系
					Edit::writePersonData(g_familyMembers, g_config.dataFile);
					UI::printSuccess("关系添加成功: " + itA->getName() + " 是 " + itB->getName() + " 的" + getRelationString(type) + "\n");
				}
				else if (opt == 2)
				{
					// 删除关系
					cout << "请输入成员ID: ";
					int pid;
					if (!(cin >> pid))
					{
						clearInput();
						cout << "ID输入无效。\n";
						continue;
					}
					auto it = std::find_if(g_familyMembers.begin(), g_familyMembers.end(),
						[pid](const Person& p) { return p.getId() == pid; });
					if (it == g_familyMembers.end())
					{
						UI::printWarn("未找到ID为 " + std::to_string(pid) + " 的家庭成员。\n");
						continue;
					}
					std::vector<Relation> rels = it->getRelations();
					if (rels.empty())
					{
						UI::printWarn("该成员没有任何关系。\n");
						continue;
					}
					cout << "该成员的关系列表:\n";
					for (size_t i = 0; i < rels.size(); i++)
					{
						cout << i + 1 << ". " << getRelationString(rels[i].type) << ": ID " << rels[i].targetId
							<< " (" << getPersonName(rels[i].targetId) << ")" << endl;
					}
					cout << "请选择要删除的关系 (0-取消): ";
					int choice;
					if (!(cin >> choice))
					{
						clearInput();
						cout << "选择无效。\n";
						continue;
					}
					if (choice <= 0 || choice > (int)rels.size())
					{
						continue;
					}
					Relation delRel = rels[choice - 1];
					it->removeRelation(delRel.type, delRel.targetId);
					// 同时删除反向关系：删除对方身上所有指向本成员的关系
					auto itTarget = std::find_if(g_familyMembers.begin(), g_familyMembers.end(),
						[delRel](const Person& p) { return p.getId() == delRel.targetId; });
					if (itTarget != g_familyMembers.end())
					{
						std::vector<Relation> targetRels = itTarget->getRelations();
						for (const auto& rel : targetRels)
						{
							if (rel.targetId == pid)
							{
								itTarget->removeRelation(rel.type, pid);
							}
						}
					}
					Edit::writePersonData(g_familyMembers, g_config.dataFile);
					UI::printSuccess("关系删除成功。\n");
				}
				else if (opt == 3)
				{
					// 查看关系
					bool any = false;
					for (auto& person : g_familyMembers)
					{
						std::vector<Relation> rels = person.getRelations();
						if (rels.empty())
						{
							continue;
						}
						any = true;
						cout << "ID: " << person.getId() << ", 姓名: " << person.getName() << endl;
						// 按类型分组显示
						for (int t = (int)RelationType::Parent; t <= (int)RelationType::Sibling; t++)
						{
							std::string names;
							int count = 0;
							for (const auto& rel : rels)
							{
								if (rel.type == (RelationType)t)
								{
									if (count > 0) names += ", ";
									names += getPersonName(rel.targetId) + " (ID " + std::to_string(rel.targetId) + ")";
									count++;
								}
							}
							if (count > 0)
							{
								cout << "  " << getRelationString((RelationType)t) << ": " << names << endl;
							}
						}
					}
					if (!any)
					{
						cout << "还没有任何亲属关系。\n";
					}
				}
			}
			continue;
		}
		if (s == 13)
		{
			// 打开网页管理系统
			openWebPage(8080);
			continue;
		}
	}
	return 0;
}
