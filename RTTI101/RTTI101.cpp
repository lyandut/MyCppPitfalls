// RTTI101.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "RTTI101.hpp"

using namespace std;

using coord_t = int;

int main() {
	std::cout << "Hello RTTI!\n";

	Rect<coord_t> rect(5, 5);
	Square<coord_t> square(5);

	Polygon<coord_t> &ref_to_rect = rect;
	Polygon<coord_t> &ref_to_square = square;

	cout << rect.equal(square) << endl;
	cout << square.equal(ref_to_square) << endl;
	cout << (rect == square) << endl;

	system("pause");

	return 0;
}
