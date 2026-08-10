#pragma once
#include <bits/stdc++.h>
class Person {

public:
	Person(int _name, std::string _birthday, bool sex);
	Person(int _name, std::string _birthday, std::string sex);
	Person(int _name, int _birthday, bool sex);
	Person(int _name, int _birthday, std::string sex);

};