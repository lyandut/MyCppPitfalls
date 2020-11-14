//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once
#ifndef MYCPPPITFALLS_RTTI101_HPP
#define MYCPPPITFALLS_RTTI101_HPP


#include <vector>
#include <string>
#include <memory>
#include <cassert>

using namespace std;

template<typename> struct Polygon;
template<typename T>
bool operator==(const Polygon<T> &lhs, const Polygon<T> &rhs) {
	return typeid(lhs) == typeid(rhs) && lhs.equal(rhs);
}
template<typename T>
bool operator!=(const Polygon<T> &lhs, const Polygon<T> &rhs) {
	return !(lhs == rhs);
}

template<typename T>
struct Polygon {
	friend bool operator==<T>(const Polygon&, const Polygon&);
	//template<typename T>
	//friend bool operator==(const Polygon&, const Polygon&);

	virtual bool equal(const Polygon &rhs) const = 0;
};

//template<typename T>
//bool operator==(const Polygon<T> &lhs, const Polygon<T> &rhs) {
//	return typeid(lhs) == typeid(rhs) && lhs.equal(rhs);
//}
//template<typename T>
//bool operator!=(const Polygon<T> &lhs, const Polygon<T> &rhs) {
//	return !(lhs == rhs);
//}

template<typename T>
struct Rect : public Polygon<T> {
	Rect(T width, T height) : width(width), height(height) {}

	bool equal(const Polygon<T> &rhs) const {
		auto r = dynamic_cast<const Rect&>(rhs);
		return width == r.width && height == r.height || width == r.height && height == r.width;
	}
	
	T width, height;
};

template <typename T>
struct Square : public Rect<T> {
	Square(T length) : Rect<T>(length, length) {}

	bool equal(const Polygon<T> &rhs) const {
		auto s = dynamic_cast<const Square&>(rhs);
		return this->width == s.width;
	}
};

#endif // !MYCPPPITFALLS_RTTI101_HPP