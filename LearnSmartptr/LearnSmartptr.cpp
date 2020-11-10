// LearnSmartptr.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "LearnSmartptr.hpp"

using namespace std;

/* 程序使用动态内存场景：
1. 程序不知道自己要使用多少对象 ===> 容器类
2. 程序不知道所需对象的准确类型 ===> 容器中放置（智能）指针而非对象
3. 程序需要在多个对象间共享数据 ===> （智能）指针成员
*/

int main() {
	std::cout << "Hello Smartptr!\n";

	vector<Point> r_points{ {0,0},{0,5},{5,5},{5,0} };
	coord_t r_width = 5, r_height = 5;
	vector<Point> c_points{ {0,0} };
	coord_t c_radius = 5;

	vector<polygon_ptr> polygon_ptrs;

	/*
	1. rect._points、polygon_ptrs[0]->_points互为拷贝，共享数据。
	2. new/make_shared会调用构造函数初始化一个新对象，因此不共享points。
	*/
	Rect rect(r_points, r_width, r_height);
	cout << "rect points成员地址:\t\t" << rect._points.get() << endl;
	cout << "rect points引用计数:\t\t" << rect._points.use_count() << endl;
	polygon_ptrs.push_back(make_shared<Rect>(rect));
	cout << "polygon_ptrs[0] points成员地址:\t" << polygon_ptrs.back()->_points.get() << endl;
	cout << "polygon_ptrs[0] points引用计数:\t" << polygon_ptrs.back()->_points.use_count() << endl;
	polygon_ptrs.push_back(make_shared<Rect>(r_points, r_width, r_height));
	cout << "polygon_ptrs[1] points成员地址:\t" << polygon_ptrs.back()->_points.get() << endl;
	cout << "polygon_ptrs[1] points引用计数:\t" << polygon_ptrs.back()->_points.use_count() << endl;
	polygon_ptrs.emplace_back(new Rect(r_points, r_width, r_height));
	cout << "polygon_ptrs[2] points成员地址:\t" << polygon_ptrs.back()->_points.get() << endl;
	cout << "polygon_ptrs[2] points引用计数:\t" << polygon_ptrs.back()->_points.use_count() << endl;

	/*
	3. 下面是一种错误的动态内存分配方式：
	   现象：编译正确运行错误，调用emplace_back没有递增引用计数，调用pop_back/erase会报错。
	   解析：rect指向的内存被rect和polygon_ptrs同时管理，会析构两次。不能析构一个并没有指向动态分配的内存空间的指针，
	         只有将动态分配的对象指针(new/make_shared)托管给shared_ptr才是有意义的，
	*/
	//polygon_ptrs.emplace_back(&rect);
	//cout << "polygon_ptrs.emplace_back(&rect) points成员地址:\t" << polygon_ptrs.back()->_points.get() << endl;
	//cout << "polygon_ptrs.emplace_back(&rect) points引用计数:\t" << polygon_ptrs.back()->_points.use_count() << endl;
	//polygon_ptrs.pop_back();

	/*
	4. 智能指针必须使用dynamic_pointer_cast下行转换。
	5. 基类必须包含虚函数，即基类是多态类型。
	*/
	polygon_ptrs.push_back(make_shared<Circle>(c_points, c_radius));
	//auto circle = dynamic_cast<Circle *>(polygon_ptrs.back());     // compile error
	//auto circle = dynamic_cast<circle_ptr>(polygon_ptrs.back());   // compile error
	auto circle = dynamic_pointer_cast<Circle>(polygon_ptrs.back()); // compile success
	cout << "polygon_ptrs[3] shape:\t\t" << circle->shape() << endl;
	cout << "polygon_ptrs[3] area:\t\t" << circle->area() << endl;

	return 0;
}
