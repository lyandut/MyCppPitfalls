// PriorityQueue.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "ShortestPath.hpp"

int main() {

	Graph myGraph(6);
	myGraph.add_edge(0, 1, 10);
	myGraph.add_edge(0, 4, 15);
	myGraph.add_edge(1, 2, 15);
	myGraph.add_edge(1, 3, 2);
	myGraph.add_edge(2, 5, 5);
	myGraph.add_edge(3, 2, 1);
	myGraph.add_edge(3, 5, 12);
	myGraph.add_edge(4, 5, 10);

	myGraph.dijkstraWithSTLQueue(0, 5);
	myGraph.dijkstraWithSTLSet(0, 5);
	myGraph.dijkstraWithCusQueue(0, 5);

	return 0;
}

