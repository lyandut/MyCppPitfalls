// LearnSmartptr.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "LearnSmartptr.hpp"

using namespace std;

static vector<Point> r_points{ {0,0},{0,5},{5,5},{5,0} };
static coord_t r_width = 5, r_height = 5;
static vector<Point> c_points{ {0,0} };
static coord_t c_radius = 5;

/*
1. 正确定义智能指针
*/
void case_1() {

	// 1. make_shared: 最安全的分配和使用动态内存的方法
	shared_ptr<Rect> p1 = make_shared<Rect>(r_points, r_width, r_height);

	Rect rect_2(r_points, r_width, r_height);
	shared_ptr<Rect> p2 = make_shared<Rect>(rect_2);

	// 2. shared_ptr和new结合使用
	shared_ptr<Rect> p3(new Rect(r_points, r_width, r_height));

	Rect *x = new Rect(r_points, r_width, r_height);
	shared_ptr<Rect> p4(x);
	x = nullptr;

	// 3. 错误用法
	// 3.1 必须直接初始化，不能从raw指针隐式转换
	//shared_ptr<Rect> p5 = new Rect(r_points, r_width, r_height); // !!!

	// 3.2 将智能指针绑定到非动态分配的内存上
	Rect rect_6(r_points, r_width, r_height);
	//shared_ptr<Rect> p6(&rect_6); // !!!
	shared_ptr<Rect> p6(&rect_6, [](Rect*) {});

	// 3.3 一份内存托管给多个智能指针
	Rect *xx = new Rect(r_points, r_width, r_height);
	shared_ptr<Rect> p7(xx);
	{
		//shared_ptr<Rect> p8(xx); // !!!
		shared_ptr<Rect> p8(xx, [](Rect*) {});
		//shared_ptr<Rect> p9(p7.get()); // !!!
		shared_ptr<Rect> p9(p7.get(), [](Rect*) {});
	}
	xx = nullptr;
	Rect rect_7 = *p7;

}

/*
2. 智能指针的使用：
   程序需要在多个对象间共享数据 ===>（智能）指针成员
*/
void case_2() {

	Rect rect_1(r_points, r_width, r_height);
	cout << "rect_1 points成员地址: " << rect_1._points.get() << endl;
	cout << "rect_1 points引用计数: " << rect_1._points.use_count() << endl;

	Rect rect_2 = rect_1;
	cout << "rect_2 points成员地址: " << rect_2._points.get() << endl;
	cout << "rect_2 points引用计数: " << rect_2._points.use_count() << endl;

}

/*
3. 智能指针的使用：
   程序不知道所需对象的准确类型 ===> 容器中放置（智能）指针而非对象
   智能指针下行转换 ===> 必须使用dynamic_pointer_cast，且基类必须是多态类型（包含虚函数）
*/
void case_3() {

	vector<polygon_ptr> polygon_ptrs;
	polygon_ptrs.push_back(make_shared<Rect>(r_points, r_width, r_height));
	polygon_ptrs.push_back(make_shared<Circle>(c_points, c_radius));

	//auto rect = dynamic_cast<Rect*>(polygon_ptrs.front()); // compile error
	//auto rect = dynamic_cast<rect_ptr>(polygon_ptrs.front()); // compile error
	auto rect = dynamic_pointer_cast<Rect>(polygon_ptrs.front()); // compile success
	cout << "polygon_ptrs.front() shape: " << rect->shape() << " area: " << rect->area() << endl;
	auto circle = dynamic_pointer_cast<Circle>(polygon_ptrs.back());
	cout << "polygon_ptrs.back() shape: " << circle->shape() << " area: " << circle->area() << endl;

}


int main() {
	std::cout << "Hello Smartptr!\n";

	case_1();

	case_2();

	case_3();

	system("pause");

	return 0;
}
