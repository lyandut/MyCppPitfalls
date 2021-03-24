//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once
#ifndef MYCPPPITFALLS_SHORTESTPATH_HPP
#define MYCPPPITFALLS_SHORTESTPATH_HPP

#include <iostream>
#include "PriorityQueue.hpp"

using std::vector;
using std::cout;
using std::endl;

struct Edge {
	int sid;
	int tid;
	int w;

	Edge(int s, int t, int w) : sid(s), tid(t), w(w) {}
};

using AdjList = vector<vector<Edge>>;

class Graph {
public:
	Graph(int v) : v_num(v), dist(v_num, INF), adj(v_num) {}

	void add_edge(int s, int t, int w) { adj[s].emplace_back(s, t, w); }

	void dijkstraWithSTLQueue(int s, int t) {
		vector<int> predecessor(v_num);
		dist.clear();
		dist.resize(v_num, INF);
		dist[s] = 0;
		PriorityQueue1 q(comp1); // 小顶堆
		q.emplace(s, 0);
		while (!q.empty()) {
			auto curr = q.top();
			q.pop();
			if (curr.id == t) { break; } // 最短路径！
			for (auto& e : adj[curr.id]) {
				if (curr.dist + e.w < dist[e.tid]) {
					predecessor[e.tid] = curr.id; // 记录前驱节点
					dist[e.tid] = curr.dist + e.w;
					q.emplace(e.tid, dist[e.tid]);
				}
			}
		}
		print_path(s, t, predecessor);
		print_dist(s, t, dist);
		cout << "priority_queue队列中剩余元素: " << q.size() << endl;
	}

	void dijkstraWithSTLSet(int s, int t) {
		vector<int> predecessor(v_num);
		dist.clear();
		dist.resize(v_num, INF);
		dist[s] = 0;
		PriorityQueue2 q(comp2);
		q.emplace(s, 0);
		while (!q.empty()) {
			auto curr = *q.begin(); // 当前最短路径出队并删除
			q.erase(q.begin());
			if (curr.id == t) { break; } // 最短路径！
			for (auto& e : adj[curr.id]) {
				if (curr.dist + e.w < dist[e.tid]) {
					predecessor[e.tid] = curr.id; // 记录前驱节点
					dist[e.tid] = curr.dist + e.w;
					for (auto iter = q.begin(); iter != q.end(); ++iter) {
						if (iter->id == e.tid) { // 如果在队列中则删除旧值
							q.erase(iter);
							break;
						}
					}
					q.emplace(e.tid, dist[e.tid]); // 新值入队
				}
			}
		}
		print_path(s, t, predecessor);
		print_dist(s, t, dist);
		cout << "set队列中剩余元素: " << q.size() << endl;
	}

	void dijkstraWithCusQueue(int s, int t) {
		vector<int> predecessor(v_num);
		dist.clear();
		dist.resize(v_num, INF);
		dist[s] = 0;
		PriorityQueue3 q(v_num);
		q.add({ s, 0 });
		vector<bool> visited(v_num, false);
		visited[s] = true; // 标记是否在队列中
		while (!q.empty()) {
			auto curr = q.poll();
			if (curr.id == t) { break; } // 最短路径！
			for (auto& e : adj[curr.id]) {
				if (dist[curr.id] + e.w < dist[e.tid]) {
					predecessor[e.tid] = curr.id;
					dist[e.tid] = dist[curr.id] + e.w;
					if (visited[e.tid]) {
						q.update({ e.tid, dist[e.tid] }); // 如果在队列中则更新dist值
					}
					else {
						q.add({ e.tid, dist[e.tid] });
						visited[e.tid] = true;
					}
				}
			}
		}
		print_path(s, t, predecessor);
		print_dist(s, t, dist);
		cout << "自定义优先队列中剩余元素: " << q.size() << endl;
	}

	void print_path(int s, int t, vector<int>& predecessor) {
		if (s == t) {
			cout << s;
			return;
		}
		print_path(s, predecessor[t], predecessor);
		cout << "->" << t;
	}

	void print_dist(int s, int t, vector<int>& dist) {
		cout << endl << s << "->" << t << ": " << dist[t] << endl;
	}

private:
	int v_num;
	vector<int> dist; // 距起点的最短路径
	AdjList adj;
};


#endif // MYCPPPITFALLS_SHORTESTPATH_HPP
