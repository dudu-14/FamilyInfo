// family.hpp
#pragma once
#include <iostream>
#include <string>

namespace FamilyInfo
{
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
		{}


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

			year = _birthday / 10000;
			month = _birthday / 100 % 100;
			day = _birthday % 100;
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
	public:
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
		}


		/// <summary>
		/// see cref="Person"/> 的一个析构函数，释放对象占用的内存
		/// </summary>
		~Person() {}
	};
}
