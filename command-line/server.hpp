// server.hpp
// 负责HTTP服务器模式，提供REST API供外部程序（如GUI）接入
// 启动方式：command-line.exe --server [端口]
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // 禁止windows.h包含winsock.h，避免与winsock2.h冲突
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "family.hpp"
#include "edit.hpp"
#include "config.hpp"
#include "log.hpp"
#include "json.hpp"

using json = nlohmann::json;
using namespace FamilyInfo;

// 全局变量声明（定义在main.cpp的全局命名空间中）
extern Config::AppConfig g_config;
extern std::vector<Person> g_familyMembers;

namespace FamilyInfo::Server
{

	/// <summary>
	/// URL解码，将%XX转义还原为原始字符
	/// </summary>
	/// <param name="str">编码后的字符串</param>
	/// <returns>解码后的字符串</returns>
	std::string urlDecode(const std::string& str)
	{
		std::string result;
		for (size_t i = 0; i < str.length(); i++)
		{
			if (str[i] == '%' && i + 2 < str.length())
			{
				int code;
				std::istringstream ss(str.substr(i + 1, 2));
				ss >> std::hex >> code;
				result += (char)code;
				i += 2;
			}
			else if (str[i] == '+')
			{
				result += ' ';
			}
			else
			{
				result += str[i];
			}
		}
		return result;
	}

	/// <summary>
	/// 获取请求路径（去掉查询参数部分）
	/// </summary>
	/// <param name="target">请求目标</param>
	/// <returns>路径</returns>
	std::string parsePath(const std::string& target)
	{
		size_t pos = target.find('?');
		return target.substr(0, pos);
	}

	/// <summary>
	/// 解析查询参数，如 /api/search?q=张三
	/// </summary>
	/// <param name="target">请求目标</param>
	/// <returns>查询参数JSON对象</returns>
	json parseQuery(const std::string& target)
	{
		json query = json::object();
		size_t pos = target.find('?');
		if (pos == std::string::npos)
		{
			return query;
		}
		std::string qs = target.substr(pos + 1);
		std::stringstream ss(qs);
		std::string pair;
		while (std::getline(ss, pair, '&'))
		{
			size_t eq = pair.find('=');
			std::string key = eq == std::string::npos ? pair : pair.substr(0, eq);
			std::string val = eq == std::string::npos ? "" : pair.substr(eq + 1);
			query[urlDecode(key)] = urlDecode(val);
		}
		return query;
	}

	/// <summary>
	/// HTTP请求结构体
	/// </summary>
	struct HttpRequest
	{
		std::string method; // 请求方法
		std::string target; // 请求目标
		std::string body;   // 请求体
	};

	/// <summary>
	/// 根据ID查找家庭成员
	/// </summary>
	/// <param name="id">成员ID</param>
	/// <returns>成员指针，找不到返回nullptr</returns>
	Person* findPerson(int id)
	{
		for (auto& person : g_familyMembers)
		{
			if (person.getId() == id)
			{
				return &person;
			}
		}
		return nullptr;
	}

	/// <summary>
	/// 根据ID获取成员姓名
	/// </summary>
	/// <param name="id">成员ID</param>
	/// <returns>姓名，找不到返回"未知"</returns>
	std::string getPersonName(int id)
	{
		Person* person = findPerson(id);
		return person ? person->getName() : "未知";
	}

	/// <summary>
	/// 将家庭成员列表转为JSON数组
	/// </summary>
	/// <param name="persons">家庭成员列表</param>
	/// <returns>JSON数组</returns>
	json personsToJson(const std::vector<Person>& persons)
	{
		json arr = json::array();
		for (auto& person : g_familyMembers)
		{
			arr.push_back(person.toJson());
		}
		return arr;
	}

	/// <summary>
	/// 解析性别，支持数字(0男1女)和字符串(man/woman/男/女)
	/// </summary>
	/// <param name="sexJson">性别JSON值</param>
	/// <returns>性别枚举</returns>
	SexEnum parseSex(const json& sexJson)
	{
		if (sexJson.is_number())
		{
			return sexJson.get<int>() == 1 ? SexEnum::Woman : SexEnum::Man;
		}
		std::string str = sexJson.get<std::string>();
		if (str == "woman" || str == "女" || str == "1")
		{
			return SexEnum::Woman;
		}
		return SexEnum::Man;
	}

	/// <summary>
	/// 发送HTTP响应
	/// </summary>
	/// <param name="client">客户端套接字</param>
	/// <param name="status">HTTP状态码</param>
	/// <param name="contentType">Content-Type</param>
	/// <param name="body">响应体</param>
	void sendResponse(SOCKET client, int status, const std::string& contentType, const std::string& body)
	{
		std::string reason = "OK";
		switch (status)
		{
		case 400: reason = "Bad Request"; break;
		case 404: reason = "Not Found"; break;
		case 405: reason = "Method Not Allowed"; break;
		case 500: reason = "Internal Server Error"; break;
		default: break;
		}
		std::ostringstream oss;
		oss << "HTTP/1.1 " << status << " " << reason << "\r\n";
		oss << "Content-Type: " << contentType << "\r\n";
		oss << "Content-Length: " << body.size() << "\r\n";
		oss << "Access-Control-Allow-Origin: *\r\n"; // 允许跨域，方便浏览器页面调试
		oss << "Connection: close\r\n";
		oss << "\r\n";
		oss << body;
		std::string response = oss.str();
		send(client, response.c_str(), (int)response.size(), 0);
	}

	/// <summary>
	/// 发送JSON错误响应
	/// </summary>
	/// <param name="client">客户端套接字</param>
	/// <param name="status">HTTP状态码</param>
	/// <param name="message">错误信息</param>
	void sendJsonError(SOCKET client, int status, const std::string& message)
	{
		json j = { {"error", message} };
		sendResponse(client, status, "application/json; charset=utf-8", j.dump());
	}

	/// <summary>
	/// 发送JSON成功响应
	/// </summary>
	/// <param name="client">客户端套接字</param>
	/// <param name="message">成功信息</param>
	void sendJsonOk(SOCKET client, const std::string& message)
	{
		json j = { {"ok", true}, {"message", message} };
		sendResponse(client, 200, "application/json; charset=utf-8", j.dump());
	}

	/// <summary>
	/// 新增家庭成员
	/// </summary>
	void handleAddPerson(SOCKET client, const std::string& body)
	{
		try
		{
			json j = json::parse(body);
			std::string name = j.value("name", "");
			std::string birthday = j.value("birthday", "");
			SexEnum sex = SexEnum::Man;
			if (j.contains("sex"))
			{
				sex = parseSex(j["sex"]);
			}
			if (name.empty())
			{
				sendJsonError(client, 400, "姓名不能为空");
				return;
			}
			Person p(name, birthday, sex);
			g_familyMembers.push_back(p);
			if (!Edit::writePersonData(g_familyMembers, g_config.dataFile))
			{
				g_familyMembers.pop_back(); // 回滚内存中的新增
				last_id--;                 // 回滚已递增的last_id
				sendJsonError(client, 500, "保存数据失败，请检查磁盘空间或文件权限");
				return;
			}
			g_config.last_id = last_id;
			Config::saveConfig(g_config);
			sendResponse(client, 200, "application/json; charset=utf-8", p.toJson().dump());
		}
		catch (const std::exception& e)
		{
			sendJsonError(client, 400, std::string("添加失败: ") + e.what());
		}
	}

	/// <summary>
	/// 修改家庭成员
	/// </summary>
	void handleUpdatePerson(SOCKET client, int id, const std::string& body)
	{
		try
		{
			Person* p = findPerson(id);
			if (!p)
			{
				sendJsonError(client, 404, "未找到该家庭成员");
				return;
			}
			json j = json::parse(body);
			if (j.contains("name"))
			{
				p->setName(j["name"].get<std::string>());
			}
			if (j.contains("birthday"))
			{
				p->setBirthday(j["birthday"].get<std::string>());
			}
			if (j.contains("sex"))
			{
				p->setSex(parseSex(j["sex"]));
			}
			if (!Edit::writePersonData(g_familyMembers, g_config.dataFile))
			{
				sendJsonError(client, 500, "保存数据失败，请检查磁盘空间或文件权限");
				return;
			}
			sendResponse(client, 200, "application/json; charset=utf-8", p->toJson().dump());
		}
		catch (const std::exception& e)
		{
			sendJsonError(client, 400, std::string("修改失败: ") + e.what());
		}
	}

	/// <summary>
	/// 删除家庭成员，同时清理他人身上指向该成员的关系
	/// </summary>
	void handleDeletePerson(SOCKET client, int id)
	{
		if (!findPerson(id))
		{
			sendJsonError(client, 404, "未找到该家庭成员");
			return;
		}
		for (auto& person : g_familyMembers)
		{
			if (person.getId() == id)
			{
				continue;
			}
			std::vector<Relation> rels = person.getRelations();
			for (const auto& rel : rels)
			{
				if (rel.targetId == id)
				{
					person.removeRelation(rel.type, id);
				}
			}
		}
		g_familyMembers.erase(std::remove_if(g_familyMembers.begin(), g_familyMembers.end(),
			[id](const Person& p) { return p.getId() == id; }), g_familyMembers.end());
		if (!Edit::writePersonData(g_familyMembers, g_config.dataFile))
		{
			sendJsonError(client, 500, "保存数据失败，请检查磁盘空间或文件权限");
			return;
		}
		sendJsonOk(client, "删除成功");
	}

	/// <summary>
	/// 列出所有亲属关系，可指定person_id只看某成员的关系
	/// </summary>
	void handleListRelations(SOCKET client, const json& query)
	{
		int filterId = query.value("person_id", 0);
		json arr = json::array();
		for (auto& person : g_familyMembers)
		{
			if (filterId > 0 && person.getId() != filterId)
			{
				continue;
			}
			for (const auto& rel : person.getRelations())
			{
				json item = {
					{"person_id", person.getId()},
					{"person_name", person.getName()},
					{"type", getRelationTypeString(rel.type)},
					{"type_name", getRelationString(rel.type)},
					{"target_id", rel.targetId},
					{"target_name", getPersonName(rel.targetId)}
				};
				arr.push_back(item);
			}
		}
		sendResponse(client, 200, "application/json; charset=utf-8", arr.dump());
	}

	/// <summary>
	/// 添加亲属关系，自动补全反向关系
	/// </summary>
	void handleAddRelation(SOCKET client, const std::string& body)
	{
		try
		{
			json j = json::parse(body);
			int personId = j.value("person_id", 0);
			int targetId = j.value("target_id", 0);
			RelationType type = parseRelationType(j.value("type", "parent"));
			Person* a = findPerson(personId);
			Person* b = findPerson(targetId);
			if (!a || !b)
			{
				sendJsonError(client, 404, "成员不存在");
				return;
			}
			if (personId == targetId)
			{
				sendJsonError(client, 400, "不能和自己建立关系");
				return;
			}
			if (a->hasRelation(type, targetId))
			{
				sendJsonError(client, 400, "该关系已存在");
				return;
			}
			a->addRelation(type, targetId);
			b->addRelation(getReverseRelation(type), personId);
			if (!Edit::writePersonData(g_familyMembers, g_config.dataFile))
			{
				sendJsonError(client, 500, "保存数据失败，请检查磁盘空间或文件权限");
				return;
			}
			sendJsonOk(client, "关系添加成功");
		}
		catch (const std::exception& e)
		{
			sendJsonError(client, 400, std::string("添加关系失败: ") + e.what());
		}
	}

	/// <summary>
	/// 删除亲属关系，同时删除对方身上所有指向本成员的反向关系
	/// </summary>
	void handleDeleteRelation(SOCKET client, const std::string& body)
	{
		try
		{
			json j = json::parse(body);
			int personId = j.value("person_id", 0);
			int targetId = j.value("target_id", 0);
			RelationType type = parseRelationType(j.value("type", "parent"));
			Person* a = findPerson(personId);
			Person* b = findPerson(targetId);
			if (!a || !b)
			{
				sendJsonError(client, 404, "成员不存在");
				return;
			}
			a->removeRelation(type, targetId);
			// 删除对方身上所有指向本成员的关系
			std::vector<Relation> targetRels = b->getRelations();
			for (const auto& rel : targetRels)
			{
				if (rel.targetId == personId)
				{
					b->removeRelation(rel.type, personId);
				}
			}
			if (!Edit::writePersonData(g_familyMembers, g_config.dataFile))
			{
				sendJsonError(client, 500, "保存数据失败，请检查磁盘空间或文件权限");
				return;
			}
			sendJsonOk(client, "关系删除成功");
		}
		catch (const std::exception& e)
		{
			sendJsonError(client, 400, std::string("删除关系失败: ") + e.what());
		}
	}

	/// <summary>
	/// 统计信息
	/// </summary>
	void handleStats(SOCKET client)
	{
		json s = json::object();
		s["total"] = (int)g_familyMembers.size();
		int man = 0;
		int woman = 0;
		long long ageSum = 0;
		int oldestAge = -1;
		int youngestAge = 999;
		json oldest = json::object();
		json youngest = json::object();
		for (auto& person : g_familyMembers)
		{
			int age = person.getAge();
			if (age < 0) age = 0;
			ageSum += age;
			if (person.getSex() == "男")
			{
				man++;
			}
			else
			{
				woman++;
			}
			if (oldestAge < 0 || age > oldestAge)
			{
				oldestAge = age;
				oldest = { {"name", person.getName()}, {"age", age} };
			}
			if (age < youngestAge)
			{
				youngestAge = age;
				youngest = { {"name", person.getName()}, {"age", age} };
			}
		}
		s["man"] = man;
		s["woman"] = woman;
		s["avg_age"] = g_familyMembers.empty() ? 0.0 : (double)ageSum / g_familyMembers.size();
		s["oldest"] = oldest;
		s["youngest"] = youngest;
		sendResponse(client, 200, "application/json; charset=utf-8", s.dump());
	}

	/// <summary>
	/// 搜索家庭成员（按姓名，不区分大小写）
	/// </summary>
	void handleSearch(SOCKET client, const json& query)
	{
		std::string keyword = query.value("q", "");
		std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
		json arr = json::array();
		for (auto& person : g_familyMembers)
		{
			std::string name = person.getName();
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);
			if (name.find(keyword) != std::string::npos)
			{
				arr.push_back(person.toJson());
			}
		}
		sendResponse(client, 200, "application/json; charset=utf-8", arr.dump());
	}

	/// <summary>
	/// 导出CSV（带UTF-8 BOM）
	/// </summary>
	void handleExport(SOCKET client)
	{
		std::string csv;
		csv += "\xEF\xBB\xBF"; // UTF-8 BOM
		csv += "ID,姓名,生日,性别,年龄\r\n";
		for (auto& person : g_familyMembers)
		{
			csv += std::to_string(person.getId()) + "," + person.getName() + "," + person.getBirthdayString()
				+ "," + person.getSex() + "," + std::to_string(person.getAge()) + "\r\n";
		}
		sendResponse(client, 200, "text/csv; charset=utf-8", csv);
	}

	/// <summary>
	/// 读取HTTP请求
	/// </summary>
	/// <param name="client">客户端套接字</param>
	/// <param name="req">请求结构体</param>
	/// <returns>读取成功返回true</returns>
	bool readRequest(SOCKET client, HttpRequest& req)
	{
		char buf[4096];
		std::string buffer;
		int received = recv(client, buf, sizeof(buf), 0);
		if (received <= 0)
		{
			return false;
		}
		buffer.assign(buf, received);

		// 解析请求行
		size_t lineEnd = buffer.find("\r\n");
		if (lineEnd == std::string::npos)
		{
			return false;
		}
		std::stringstream ss(buffer.substr(0, lineEnd));
		ss >> req.method >> req.target;

		// 解析请求头，获取Content-Length
		int contentLength = 0;
		size_t headerEnd = buffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos)
		{
			std::string headers = buffer.substr(lineEnd + 2, headerEnd - lineEnd - 2);
			std::stringstream hs(headers);
			std::string line;
			while (std::getline(hs, line))
			{
				if (!line.empty() && line.back() == '\r')
				{
					line.pop_back();
				}
				std::string lower = line;
				std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
				if (lower.find("content-length:") == 0)
				{
					contentLength = std::stoi(line.substr(line.find(':') + 1));
				}
			}
			// 读取请求体
			std::string body = buffer.substr(headerEnd + 4);
			while ((int)body.size() < contentLength)
			{
				received = recv(client, buf, sizeof(buf), 0);
				if (received <= 0)
				{
					break;
				}
				body.append(buf, received);
			}
			req.body = body.substr(0, contentLength);
		}
		return true;
	}

	/// <summary>
	/// 处理单个请求，按路径路由
	/// </summary>
	void handleRequest(SOCKET client, const HttpRequest& req)
	{
		std::string path = parsePath(req.target);
		json query = parseQuery(req.target);
		Log::logInfo("收到请求: " + req.method + " " + path);
		try
		{
			// 每次请求都重新加载数据，保证与数据文件一致
			Edit::loadPersonData(g_familyMembers, g_config.dataFile);

			if (path == "/api/persons")
			{
				if (req.method == "GET")
				{
					sendResponse(client, 200, "application/json; charset=utf-8", personsToJson(g_familyMembers).dump());
					return;
				}
				if (req.method == "POST")
				{
					handleAddPerson(client, req.body);
					return;
				}
				sendJsonError(client, 405, "方法不支持");
				return;
			}
			if (path.rfind("/api/persons/", 0) == 0)
			{
				std::string idStr = path.substr(std::string("/api/persons/").length());
				int id = std::atoi(idStr.c_str());
				if (req.method == "GET")
				{
					Person* p = findPerson(id);
					if (!p)
					{
						sendJsonError(client, 404, "未找到该家庭成员");
						return;
					}
					sendResponse(client, 200, "application/json; charset=utf-8", p->toJson().dump());
					return;
				}
				if (req.method == "PUT")
				{
					handleUpdatePerson(client, id, req.body);
					return;
				}
				if (req.method == "DELETE")
				{
					handleDeletePerson(client, id);
					return;
				}
				sendJsonError(client, 405, "方法不支持");
				return;
			}
			if (path == "/api/relations")
			{
				if (req.method == "GET")
				{
					handleListRelations(client, query);
					return;
				}
				if (req.method == "POST")
				{
					handleAddRelation(client, req.body);
					return;
				}
				if (req.method == "DELETE")
				{
					handleDeleteRelation(client, req.body);
					return;
				}
				sendJsonError(client, 405, "方法不支持");
				return;
			}
			if (path == "/api/stats" && req.method == "GET")
			{
				handleStats(client);
				return;
			}
			if (path == "/api/search" && req.method == "GET")
			{
				handleSearch(client, query);
				return;
			}
			if (path == "/api/export" && req.method == "GET")
			{
				handleExport(client);
				return;
			}
			sendJsonError(client, 404, "接口不存在: " + path);
		}
		catch (const std::exception& e)
		{
			Log::logError(std::string("处理请求异常: ") + e.what());
			sendJsonError(client, 500, std::string("服务器内部错误: ") + e.what());
		}
	}

	/// <summary>
	/// 启动HTTP服务器
	/// </summary>
	/// <param name="port">监听端口</param>
	/// <returns>退出码</returns>
	int runServer(int port = 8080)
	{
		Log::logInfo("启动HTTP服务器模式，端口: " + std::to_string(port));
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << "[ERROR] WSAStartup失败" << std::endl;
			Log::logError("WSAStartup失败");
			return 1;
		}
		SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket == INVALID_SOCKET)
		{
			std::cerr << "[ERROR] 创建套接字失败" << std::endl;
			Log::logError("创建套接字失败");
			WSACleanup();
			return 1;
		}
		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_port = htons((u_short)port);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
		if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			std::cerr << "[ERROR] 绑定端口失败: " << port << "（可能已被占用）" << std::endl;
			Log::logError("绑定端口失败: " + std::to_string(port));
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		if (listen(listenSocket, 5) == SOCKET_ERROR)
		{
			std::cerr << "[ERROR] 监听失败" << std::endl;
			Log::logError("监听失败");
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		std::cout << "家庭信息管理系统服务器已启动: http://127.0.0.1:" << port << std::endl;
		std::cout << "按 Ctrl+C 停止服务" << std::endl;
		while (1)
		{
			// 等待客户端连接
			SOCKET client = accept(listenSocket, nullptr, nullptr);
			if (client == INVALID_SOCKET)
			{
				continue;
			}
			HttpRequest req;
			if (readRequest(client, req))
			{
				handleRequest(client, req);
			}
			closesocket(client);
		}
		closesocket(listenSocket);
		WSACleanup();
		return 0;
	}
}
