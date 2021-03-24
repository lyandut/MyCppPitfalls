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
1. STL���ȶ��� priority_queue
   priority_queue��һ����������������������vector/deque�����ڶѻ�����(make_heap)ʵ�֣�
   ���˸о�STLĬ�ϴ󶥶��е㷴����...
*/
std::function<bool(const Vertex&, const Vertex&)> comp1 = [](const Vertex& lhs, const Vertex& rhs) {
	if (lhs.dist == rhs.dist) return lhs.id > rhs.id;
	return lhs.dist > rhs.dist;
};
using PriorityQueue1 = std::priority_queue<Vertex, std::vector<Vertex>, decltype(comp1)>;


/*
2. STL�������� set
   set���ں����ʵ�֣��������ܺã����Ե���`֧��ɾ��ָ��Ԫ�ص����ȶ���`ʹ�ã�
   ͬ��Ҫ��Ԫ��֧�ֱȽϺ���.
*/
auto comp2 = [](auto& lhs, auto& rhs) {
	if (lhs.dist == rhs.dist) return lhs.id < rhs.id;
	return lhs.dist < rhs.dist;
};
using PriorityQueue2 = std::set<Vertex, decltype(comp2)>;

//using PriorityQueue2 = std::set<Vertex>; // Vertex����`<`�����


/*
3. �Զ���`֧�ָ���ָ��Ԫ�ص����ȶ���`
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
			heapify_float(i); // С���ϸ�
		}
		else {
			nodes[i].dist = data.dist;
			heapify_sink(i); // ����³�
		}
	}

	bool empty() const { return count == 0; }

	int size() const { return count; }

private:
	// �������Ͻ��ѣ�С���ϸ�
	void heapify_float(int i) {
		while (i / 2 > 0 && nodes[i].dist < nodes[i / 2].dist) {
			std::swap(nodes[i], nodes[i / 2]);
			i /= 2;
		}
	}

	// �������¶ѻ�������³�
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
