// family.hpp
#pragma once
#include <iostream>
#include <stdexcept>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
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
		int getAge() {
			boost::gregorian::date today = boost::gregorian::day_clock::local_day();
			age = today.year() - birthday.year;
			if (today.month() < birthday.month || (today.month() == birthday.month && today.day() < birthday.day)) {
				age--;
			}
			return age;
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

		json toJson()
		{
			json j;
			j["id"] = id;
			j["name"] = name;
			j["birthday"] = getBirthdayString();
			j["sex"] = sex ? "woman" : "man"; // 存储用英文，避免编码问题
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
