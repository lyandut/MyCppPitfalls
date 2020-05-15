//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once
#ifndef MYCPPPITFALLS_GETANDGETLINE_HPP
#define MYCPPPITFALLS_GETANDGETLINE_HPP

#include <iostream>
#include <sstream>
#include <vector>
#include <array>
#include <string>

/** std::basic_istream<CharT,Traits>::get
	[1] int_type get();
	[2] basic_istream& get( char_type& ch );
	[3] basic_istream& get( char_type* s, std::streamsize count );
	[4] basic_istream& get( char_type* s, std::streamsize count, char_type delim );
	[5] basic_istream& get( basic_streambuf& strbuf );
	[6] basic_istream& get( basic_streambuf& strbuf, char_type delim );
***/
static void testGet() {
	std::istringstream s1("Hello, world.\nCannot read");
	std::istringstream s2("123\n|Cannot read");

	// [1]
	char c1 = s1.get(); // reads 'H'
	std::cout << "[1] After reading " << c1 << ", gcount() == " << s1.gcount() << '\n';

	// [2]
	char c2;
	s1.get(c2); // reads 'e'

	// [3]
	char str1[5];
	s1.get(str1, 5); // reads "llo,"
	std::cout << "[3] After reading " << str1 << ", gcount() == " << s1.gcount() << '\n';

	std::cout << c1 << c2 << str1;

	// [5] 读取剩余字符，读到 '\n' 停止 
	s1.get(*std::cout.rdbuf()); // reads " world."
	std::cout << "\n[5] After the last get(), gcount() == " << s1.gcount() << '\n';

	// [4] 读到 '|' 停止，'|' 未释放仍保存在流中
	char str2[10];
	s2.get(str2, 10, '|'); // reads "123\n"
	std::cout << "[4]After reading " << str2 << ", gcount() == " << s2.gcount() << '\n';

	// [6] 读到 '|' 停止
	s2.get(*std::cout.rdbuf(), '|'); // reads nothing
	std::cout << "[6]After the last get(), gcount() == " << s2.gcount() << '\n';
}


/** std::basic_istream<CharT,Traits>::getline
	[1] basic_istream& getline( char_type* s, std::streamsize count );
	[2] basic_istream& getline( char_type* s, std::streamsize count, char_type delim );
***/
static void testGetline_istream() {
	std::istringstream input1("abc\ndef\ngh");
	std::istringstream input2("123|456|\n78");
	std::vector<std::array<char, 4>> v;

	// [1]
	for (std::array<char, 4> a; input1.getline(&a[0], 4);) {
		v.push_back(a);
	}
	for (auto& a : v) {
		std::cout << &a[0] << '\n';
	}

	v.clear();

	// [2]
	for (std::array<char, 4> a; input2.getline(&a[0], 4, '|'); ) {
		v.push_back(a);
	}
	for (auto& a : v) {
		std::cout << &a[0] << '\n';
	}
}


/** std::getline
	[1] getline( input, str, delim );
	[2] getline( input, str );
***/
static void testGetline_string() {
	// [2]
	std::string name;
	std::cout << "What is your name? ";
	std::getline(std::cin, name);
	std::cout << "Hello " << name << ", nice to meet you.\n";

	// [1]
	std::istringstream input;
	input.str("1|2|3|4|5|6|7|");
	int sum = 0;
	for (std::string line; std::getline(input, line, '|'); ) {
		sum += std::stoi(line);
	}
	std::cout << "\nThe sum is: " << sum << "\n";
}


/// basic_istream& getline( char_type* s, std::streamsize count, char_type delim );
/// [cppreference] `count-1` characters have been extracted (in which case `setstate(failbit)` is executed).
/// [warning] 为避免错误，字符数组必须要预留足够的空间！！！
static void testGetline_failbit() {
	std::istringstream input("123|4567|89");

	// note: the following loop terminates when std::ios_base::operator bool()
	// on the stream returned from getline() returns false
	std::array<char, 4> a;
	while (input.getline(&a[0], 4, '|')) {
		// 读取 "123" 同时 '|' 也被读出
		std::cout << "After reading " << &a[0] << ", gcount() == " << input.gcount() << '\n'; // 4
		std::cout << "After reading " << &a[0] << ", tellg() == " << input.tellg() << '\n'; // 4
	}

	// 读取 "456" 时字符数组读满导致 setstate(failbit) 执行，跳出循环
	std::cout << "After reading " << &a[0] << ", gcount() == " << input.gcount() << '\n'; // 3
	std::cout << "After reading " << &a[0] << ", tellg() == " << input.tellg() << '\n'; // -1
	std::cout << "After reading " << &a[0] << ", rdstate() == " << input.rdstate() << '\n'; // 2 == failbit

	// 设置状态为 goodbit 可找到循环退出时文件流指针的确切位置
	input.clear();
	std::cout << "After clear(), tellg() == " << input.tellg() << '\n'; // 7

	// 移到指针到文件开头
	input.seekg(0, std::ios::beg);
	std::cout << "After seekg(), tellg() == " << input.tellg() << '\n'; // 0
}


static void testGetlineAndIstream() {
	std::istringstream input("123\n***\n456");
	int a, b;
	std::string str;

	input >> a;
	std::cout << "First reading from stream: " << a << '\n';
	std::getline(input, str);
	std::getline(input, str);
	input >> b;
	std::cout << "Second reading from stream: " << b << '\n';
}

#endif // !MYCPPPITFALLS_GETANDGETLINE_HPP
