//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once
#ifndef MYCPPPITFALLS_PRIORITYQUEUE_HPP
#define MYCPPPITFALLS_PRIORITYQUEUE_HPP

#include <set>
#include <functional>


std::function<bool(int, int)> comp = [](int lhs, int rhs) {
	return lhs > rhs;
};
std::set<int, decltype(comp)> priority_queue_1;





#endif // MYCPPPITFALLS_PRIORITYQUEUE_HPP
