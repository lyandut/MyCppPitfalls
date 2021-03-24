//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once
#ifndef MYCPPPITFALLS_PRIORITYQUEUE_HPP
#define MYCPPPITFALLS_PRIORITYQUEUE_HPP

#define INF 0x3f3f3f3f

#include <vector>
#include <queue>
#include <set>
#include <functional>


struct Vertex {
	int id;
	int dist;

	Vertex() : id(-1), dist(INF) {}
	Vertex(int i, int d) : id(i), dist(d) {}
	bool operator<(const Vertex& rhs) const {
		if (dist == rhs.dist) return id < rhs.id;
		return dist < rhs.dist;
	}
};


/*
1. STL优先队列 priority_queue
   priority_queue是一种容器适配器，可以适配vector/deque，基于堆化操作(make_heap)实现；
   个人感觉STL默认大顶堆有点反人类...
*/
std::function<bool(const Vertex&, const Vertex&)> comp1 = [](const Vertex& lhs, const Vertex& rhs) {
	if (lhs.dist == rhs.dist) return lhs.id > rhs.id;
	return lhs.dist > rhs.dist;
};
using PriorityQueue1 = std::priority_queue<Vertex, std::vector<Vertex>, decltype(comp1)>;


/*
2. STL有序容器 set
   set基于红黑树实现，查找性能好，可以当作`支持删除指定元素的优先队列`使用；
   同样要求元素支持比较函数.
*/
auto comp2 = [](auto& lhs, auto& rhs) {
	if (lhs.dist == rhs.dist) return lhs.id < rhs.id;
	return lhs.dist < rhs.dist;
};
using PriorityQueue2 = std::set<Vertex, decltype(comp2)>;

//using PriorityQueue2 = std::set<Vertex>; // Vertex重载`<`运算符


/*
3. 自定义`支持更新指定元素的优先队列`
*/
class PriorityQueue3 {
public:
	PriorityQueue3(int c) : capacity(c), count(0) {
		nodes.resize(capacity + 1);
	}

	void add(Vertex&& data) {
		if (count >= capacity) return;
		++count;
		nodes[count] = data;
		heapify_float(count);
	}

	Vertex poll() {
		if (count == 0) return {};
		Vertex top = nodes[1];
		nodes[1] = nodes[count];
		--count;
		heapify_sink(1);
		return top;
	}

	void update(Vertex&& data) {
		int i = 1;
		for (; i <= count; ++i) {
			if (nodes[i].id == data.id) { break; }
		}
		if (nodes[i].dist > data.dist) {
			nodes[i].dist = data.dist;
			heapify_float(i); // 小的上浮
		}
		else {
			nodes[i].dist = data.dist;
			heapify_sink(i); // 大的下沉
		}
	}

	bool empty() const { return count == 0; }

	int size() const { return count; }

private:
	// 自下往上建堆，小的上浮
	void heapify_float(int i) {
		while (i / 2 > 0 && nodes[i].dist < nodes[i / 2].dist) {
			std::swap(nodes[i], nodes[i / 2]);
			i /= 2;
		}
	}

	// 自上往下堆化，大的下沉
	void heapify_sink(int i) {
		while (true) {
			int min_pos = i;
			if (i * 2 <= count && nodes[i * 2].dist < nodes[min_pos].dist) min_pos = i * 2;
			if (i * 2 + 1 <= count && nodes[i * 2 + 1].dist < nodes[min_pos].dist) min_pos = i * 2 + 1;
			if (min_pos == i) break;
			std::swap(nodes[min_pos], nodes[i]);
			i = min_pos;
		}
	}

private:
	std::vector<Vertex> nodes;
	int capacity;
	int count;
};

#endif // MYCPPPITFALLS_PRIORITYQUEUE_HPP
