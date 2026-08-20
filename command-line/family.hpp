// family.hpp
#pragma once
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <ctime>
#include "json.hpp"

using json = nlohmann::json;
namespace FamilyInfo
{

	int last_id = 0;

	/// <summary>
	/// 性别枚举类型，0男 1女
	/// </summary>
	enum SexEnum
	{
		Man,
		Woman
	};
	/// <summary>
	/// 获取性别的字符串
	/// </summary>
	/// <param name="sex">性别枚举</param>
	/// <returns>字符串，"男"或"女"</returns>
	std::string getSexString(SexEnum sex)
	{
		return sex ? "女" : "男";
	}

	/// <summary>
	/// 亲属关系类型，Parent为子女关系的反向（父母）
	/// </summary>
	enum RelationType
	{
		Parent,  // 父母
		Father,  // 父亲
		Mother,  // 母亲
		Spouse,  // 配偶
		Child,   // 子女
		Sibling  // 兄弟姐妹
	};

	/// <summary>
	/// 获取关系类型的中文名称
	/// </summary>
	/// <param name="type">关系类型</param>
	/// <returns>中文名称</returns>
	std::string getRelationString(RelationType type)
	{
		switch (type)
		{
		case RelationType::Father: return "父亲";
		case RelationType::Mother: return "母亲";
		case RelationType::Spouse: return "配偶";
		case RelationType::Child: return "子女";
		case RelationType::Sibling: return "兄弟姐妹";
		default: return "父母";
		}
	}

	/// <summary>
	/// 获取关系类型在JSON中存储的英文名称
	/// </summary>
	/// <param name="type">关系类型</param>
	/// <returns>英文名称</returns>
	std::string getRelationTypeString(RelationType type)
	{
		switch (type)
		{
		case RelationType::Father: return "father";
		case RelationType::Mother: return "mother";
		case RelationType::Spouse: return "spouse";
		case RelationType::Child: return "child";
		case RelationType::Sibling: return "sibling";
		default: return "parent";
		}
	}

	/// <summary>
	/// 根据英文名称解析关系类型
	/// </summary>
	/// <param name="str">英文名称</param>
	/// <returns>关系类型</returns>
	RelationType parseRelationType(const std::string& str)
	{
		if (str == "father") return RelationType::Father;
		if (str == "mother") return RelationType::Mother;
		if (str == "spouse") return RelationType::Spouse;
		if (str == "child") return RelationType::Child;
		if (str == "sibling") return RelationType::Sibling;
		return RelationType::Parent;
	}

	/// <summary>
	/// 获取反向关系类型，添加/删除关系时自动补全反向关系
	/// </summary>
	/// <param name="type">关系类型</param>
	/// <returns>反向关系类型</returns>
	/// <remarks>
	/// 父亲/母亲的反向是子女，子女的反向是父母，配偶/兄弟姐妹的反向是自身。
	/// </remarks>
	RelationType getReverseRelation(RelationType type)
	{
		switch (type)
		{
		case RelationType::Father:
		case RelationType::Mother:
		case RelationType::Parent: return RelationType::Child;
		case RelationType::Child: return RelationType::Parent;
		default: return type; // 配偶、兄弟姐妹
		}
	}

	/// <summary>
	/// 一条亲属关系，表示"本成员 是 对方的某类亲属"
	/// </summary>
	struct Relation
	{
		RelationType type; // 关系类型
		int targetId;      // 对方成员的ID
	};


	/// <summary>
	/// 判断是否为闰年
	/// </summary>
	/// <param name="year">年份</param>
	/// <returns>闰年返回true，否则返回false</returns>
	bool isLeapYear(int year)
	{
		return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
	}

	/// <summary>
	/// 判断年月日是否为有效日期
	/// </summary>
	/// <param name="year">年份</param>
	/// <param name="month">月份</param>
	/// <param name="day">日期</param>
	/// <returns>有效返回true，否则返回false</returns>
	bool isValidDate(int year, int month, int day)
	{
		if (year < 1 || month < 1 || month > 12 || day < 1)
		{
			return false;
		}
		int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		if (month == 2 && isLeapYear(year))
		{
			daysInMonth[1] = 29;
		}
		return day <= daysInMonth[month - 1];
	}

	/// <summary>
	/// 生日类型，用于存储出生年、月、日
	/// </summary>
	/// <remarks>
	/// 有三种构造函数，分别用于初始化空对象、字符串日期和整数日期。
	/// </remarks>
	class BirthdayClass
	{
	public:
		/// <summary>
		/// 空的 <see cref="BirthdayClass"/> 构造函数，预留
		/// </summary>
		BirthdayClass() {}


		/// <summary>
		/// 初始化生日对象
		/// </summary>
		/// <param name="_birthday">生日字符串，格式为YYYY-MM-DD</param>
		/// <returns>无返回值</returns>
		/// <remarks>
		/// 此函数用于将YYYY-MM-DD格式的字符串日期解析为年、月、日。
		/// </remarks>
		BirthdayClass(std::string _birthday)
		{
			// 校验日期格式，必须为YYYY-MM-DD
			if (_birthday.length() != 10 || _birthday[4] != '-' || _birthday[7] != '-')
			{
				throw std::invalid_argument("生日格式不正确，应为YYYY-MM-DD");
			}
			int y = (std::stoi(_birthday.substr(0, 4)));
			int m = (std::stoi(_birthday.substr(5, 2)));
			int d = (std::stoi(_birthday.substr(8, 2)));
			// 校验是否为真实存在的日期
			if (!isValidDate(y, m, d))
			{
				throw std::invalid_argument("生日不是有效日期");
			}
			this->year = y;
			this->month = m;
			this->day = d;
		}


		/// <summary>
		/// 初始化生日对象
		/// </summary>
		/// <param name="_birthday">生日整数，格式为YYYYMMDD</param>
		/// <returns>无返回值</returns>
		/// <remarks>
		/// 此函数用于将YYYYMMDD格式的整数日期解析为年、月、日。
		/// </remarks>
		BirthdayClass(int _birthday)
		{
			int y = _birthday / 10000;
			int m = _birthday / 100 % 100;
			int d = _birthday % 100;
			// 校验是否为真实存在的日期
			if (!isValidDate(y, m, d))
			{
				throw std::invalid_argument("生日不是有效日期");
			}
			year = y;
			month = m;
			day = d;
		}
		int year, month, day;
	};
	/// <summary>
	/// 人对象 含有一个id、姓名、生日、性别
	/// </summary>
	/// <remarks>
	/// 基本对象，用于存储家庭成员的基本信息。
	/// </remarks>
	class Person
	{
	private:
		/// <summary>
		/// 对象ID变量，int类型
		/// 目前想不到什么用，先留着吧
		/// </summary>
		int id;
		/// <summary>
		/// 对象姓名变量，string类型
		/// </summary>
		std::string name;
		/// <summary>
		/// 生日变量，自定义birthday类型，对象public变量获得出生年月日
		/// </summary>
		BirthdayClass birthday;
		/// <summary>
		/// 性别枚举，0男 1女  <code>getSexString(sex)</code>获得字符串
		/// </summary>
		SexEnum sex;
		/// <summary>
		/// 年龄，获取年龄时自动计算
		/// </summary>
		int age;
		/// <summary>
		/// 亲属关系列表，记录本成员与其他成员的关系
		/// </summary>
		std::vector<Relation> relations;
		/// <summary>
		/// 上一个id，用于生成新的id
		/// </summary>
	public:

		//各种get
		int getId() const { return id; }
		std::string getName() const { return name; }
		BirthdayClass getBirthday() const { return birthday; }
		/// <summary>
		/// 获取生日字符串，格式为YYYY-MM-DD，月份和日期不足两位自动补零
		/// </summary>
		/// <returns>生日字符串</returns>
		std::string getBirthdayString() const {
			std::string str = std::to_string(birthday.year) + "-";
			if (birthday.month < 10) str += "0";
			str += std::to_string(birthday.month) + "-";
			if (birthday.day < 10) str += "0";
			str += std::to_string(birthday.day);
			return str;
		}
		std::string getSex() const { return getSexString(sex); }
		int getAge() const {
			// 获取今天的年月日（使用标准库，无第三方依赖）
			std::time_t now = std::time(nullptr);
			std::tm tmNow{};
			localtime_s(&tmNow, &now);
			int currentAge = (tmNow.tm_year + 1900) - birthday.year;
			if ((tmNow.tm_mon + 1) < birthday.month || ((tmNow.tm_mon + 1) == birthday.month && tmNow.tm_mday < birthday.day)) {
				currentAge--;
			}
			return currentAge;
		}

		//各种set
		/// <summary>
		/// 设置对象ID，加载数据时用于恢复原本的ID
		/// </summary>
		/// <param name="_id">ID值</param>
		void setId(int _id) { this->id = _id; }
		/// <summary>
		/// 设置对象姓名
		/// </summary>
		/// <param name="_name">姓名</param>
		void setName(const std::string& _name) { this->name = _name; }
		/// <summary>
		/// 设置对象生日
		/// </summary>
		/// <param name="_birthday">生日字符串，格式为YYYY-MM-DD</param>
		void setBirthday(const std::string& _birthday) { this->birthday = BirthdayClass(_birthday); }
		/// <summary>
		/// 设置对象性别
		/// </summary>
		/// <param name="_sex">性别枚举</param>
		void setSex(SexEnum _sex) { this->sex = _sex; }

		//各种关系
		/// <summary>
		/// 添加一条亲属关系
		/// </summary>
		/// <param name="type">关系类型</param>
		/// <param name="targetId">对方成员的ID</param>
		void addRelation(RelationType type, int targetId) { relations.push_back({ type, targetId }); }

		/// <summary>
		/// 删除一条亲属关系
		/// </summary>
		/// <param name="type">关系类型</param>
		/// <param name="targetId">对方成员的ID</param>
		/// <returns>删除成功返回true，关系不存在返回false</returns>
		bool removeRelation(RelationType type, int targetId)
		{
			for (auto it = relations.begin(); it != relations.end(); ++it)
			{
				if (it->type == type && it->targetId == targetId)
				{
					relations.erase(it);
					return true;
				}
			}
			return false;
		}

		/// <summary>
		/// 判断某条亲属关系是否已存在
		/// </summary>
		/// <param name="type">关系类型</param>
		/// <param name="targetId">对方成员的ID</param>
		/// <returns>存在返回true，否则返回false</returns>
		bool hasRelation(RelationType type, int targetId) const
		{
			for (const auto& rel : relations)
			{
				if (rel.type == type && rel.targetId == targetId)
				{
					return true;
				}
			}
			return false;
		}

		/// <summary>
		/// 获取所有亲属关系
		/// </summary>
		/// <returns>亲属关系列表</returns>
		std::vector<Relation> getRelations() const { return relations; }

		/// <summary>
		/// 设置亲属关系列表，加载数据时使用
		/// </summary>
		/// <param name="rels">亲属关系列表</param>
		void setRelations(const std::vector<Relation>& rels) { this->relations = rels; }

		json toJson()
		{
			json j;
			j["id"] = id;
			j["name"] = name;
			j["birthday"] = getBirthdayString();
			j["sex"] = sex ? "woman" : "man"; // 存储用英文，避免编码问题
			j["relations"] = json::array();
			for (const auto& rel : relations)
			{
				j["relations"].push_back({ {"type", getRelationTypeString(rel.type)}, {"target_id", rel.targetId} });
			}
			return j;
		}

		/// <summary>
		/// 构造 <see cref="Person"/> class.
		/// </summary>
		/// <param name="_name">姓名.</param>
		/// <param name="_birthday">生日，string类型.</param>
		/// <param name="sex">性别.</param>
		Person(std::string _name, std::string _birthday, SexEnum sex)
		{
			this->name = _name;
			this->birthday = BirthdayClass(_birthday);
			this->sex = sex;
			this->id = ++last_id;
		}


		/// <summary>
		/// 构造 <see cref="Person"/> class.
		/// </summary>
		/// <param name="_name">姓名.</param>
		/// <param name="_birthday">生日，string类型.</param>
		/// <param name="sex">性别.</param>
		Person(std::string _name, int _birthday, SexEnum sex)
		{
			this->name = _name;
			this->birthday = BirthdayClass(_birthday);
			this->sex = sex;
			this->id = ++last_id;
		}

		/// <summary>
		/// 构造 <see cref="Person"/> class.
		/// </summary>
		/// <param name="_id">ID.</param>
		/// <param name="_name">姓名.</param>
		/// <param name="_birthday">生日，string类型.</param>
		/// <param name="_sex">性别.</param>
		/// <remarks>
		/// 直接指定ID，不递增全局last_id，用于从文件加载数据时恢复原本的ID。
		/// </remarks>
		Person(int _id, std::string _name, std::string _birthday, SexEnum _sex)
		{
			this->id = _id;
			this->name = _name;
			this->birthday = BirthdayClass(_birthday);
			this->sex = _sex;
		}

		Person() {};

		/// <summary>
		/// see cref="Person"/> 的一个析构函数，释放对象占用的内存
		/// </summary>
		~Person() {}
	};
}
