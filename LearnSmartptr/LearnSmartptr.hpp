//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once
#ifndef MYCPPPITFALLS_LEARNSMARTPTR_HPP
#define MYCPPPITFALLS_LEARNSMARTPTR_HPP

#include <vector>
#include <string>
#include <memory>
#include <cassert>

using namespace std;

static constexpr double PI = 3.14;

using coord_t = double;

struct Point { coord_t x, y; };

class Polygon {
public:
	Polygon(const vector<Point> &points) :
		_points(make_shared<vector<Point>>(points)) {}

	virtual string shape() const = 0;

	virtual coord_t area() const = 0;

public:
	const shared_ptr<vector<Point>> _points;
};

class Rect final : public Polygon {
public:
	Rect(const vector<Point> &points, coord_t width, coord_t height) :
		Polygon(points), _width(width), _height(height) {
		assert(points.size() == 4);
	}

	string shape() const { return "Rect"; }

	coord_t area() const { return _width * _height; }

private:
	const coord_t _width;
	const coord_t _height;
};

class Circle final : public Polygon {
public:
	Circle(const vector<Point> &points, coord_t radius) :
		Polygon(points), _center(points.front()), _radius(radius) {
		assert(points.size() == 1);
	}

	string shape() const { return "Circle"; }

	coord_t area() const { return PI * _radius * _radius; }

private:
	const Point _center;
	const coord_t _radius;
};

using polygon_ptr = shared_ptr<Polygon>;

using rect_ptr = shared_ptr<Rect>;

using circle_ptr = shared_ptr<Circle>;

#endif // !MYCPPPITFALLS_LEARNSMARTPTR_HPP