//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once
#ifndef MYCPPPITFALLS_LEARNGUROBI_HPP
#define MYCPPPITFALLS_LEARNGUROBI_HPP

#include <vector>
#include <gurobi_c++.h>

using namespace std;

void gurobi_cluster(vector<int> vex_wgts, vector<vector<int>> graph, int group_num) {

	int node_num = vex_wgts.size();

	try {
		// Initialize environment & empty model
		GRBEnv env = GRBEnv(true);
		env.set("LogFile", "cluster.log");
		env.start();
		GRBModel gm = GRBModel(env);
		gm.set(GRB_DoubleParam_TimeLimit, 60.0 * 10);

		// Decision Variables
		// x_ip ∈{0,1}: node_i是否被分到group_p.
		// y_ijp∈{0,1}: edge_ij是否被分到group_p.
		// z: 最小分组权重和.
		vector<vector<GRBVar>> x(node_num, vector<GRBVar>(group_num));
		for (int i = 0; i < x.size(); ++i) {
			for (int p = 0; p < x[i].size(); ++p) {
				x[i][p] = gm.addVar(0, 1, 0, GRB_BINARY);
			}
		}
		vector<vector<vector<GRBVar>>> y(node_num, vector<vector<GRBVar>>(node_num, vector<GRBVar>(group_num)));
		for (int i = 0; i < y.size(); ++i) {
			for (int j = 0; j < y[i].size(); ++j) {
				for (int p = 0; p < y[i][j].size(); ++p) {
					y[i][j][p] = gm.addVar(0, 1, 0, GRB_BINARY);
				}
			}
		}
		GRBVar z = gm.addVar(0, 1, GRB_INFINITY, GRB_INTEGER);

		// Constraint
		// Sum(x_ip) = 1: node_i一定要被分配到某一个group_p中.
		// Sum(x_ip * v_i) >= z: z是最小分组权重和.
		// y_ijp = x_ip ∧ x_jp: node_i和node_j都分配到group_p中，则edge_ij也被分配到p中.
		for (int i = 0; i < node_num; ++i) {
			GRBLinExpr sum_xip = 0;
			for (int p = 0; p < group_num; ++p) { sum_xip += x[i][p]; }
			gm.addConstr(sum_xip, GRB_EQUAL, 1);
		}
		for (int p = 0; p < group_num; ++p) {
			GRBLinExpr sum_xip_vi = 0;
			for (int i = 0; i < node_num; ++i) { sum_xip_vi += x[i][p] * vex_wgts[i]; }
			gm.addConstr(sum_xip_vi >= z);
		}
		for (int p = 0; p < group_num; ++p) {
			for (int i = 0; i < node_num; ++i) {
				for (int j = 0; j < node_num; ++j) {
					if (i == j) { continue; }
					GRBLinExpr expr = x[i][p] + x[j][p] - 2 * y[i][j][p];
					gm.addConstr(expr >= 0);
					//gm.addConstr(expr <= 1); // 提速：由于是最大化目标，可以删除该条约束；同理最小化目标则可删除上条约束.
				}
			}
		}

		// Objective Function
		// maximize Sum(y_ijp * w_ij): 最大化p个分组的边权重之和.
		// maximize z: 最大化z.
		// gurobi要求多个目标函数必须具有相同sense，即优化方向同为最大（小）.
		GRBLinExpr obj = 0;
		for (int p = 0; p < group_num; ++p) {
			for (int i = 0; i < node_num; ++i) {
				for (int j = 0; j < node_num; ++j) {
					if (i == j) { continue; }
					obj += y[i][j][p] * graph[i][j];
				}
			}
		}
		gm.setObjective(obj, GRB_MAXIMIZE);
		gm.setObjectiveN();

		// Optimize model
		gm.optimize();
		int status = gm.get(GRB_IntAttr_Status);
		if (status == GRB_OPTIMAL || status == GRB_TIME_LIMIT) {
			for (int i = 0; i < x.size(); ++i) {
				for (int p = 0; p < x[i].size(); ++p) {
					if (x[i][p].get(GRB_DoubleAttr_X)) {
						std::cout << i << " is in group " << p << std::endl;
					}
				}
			}
			std::cout << "Obj: " << gm.get(GRB_DoubleAttr_ObjVal) << std::endl;
		}
		else if (status == GRB_INFEASIBLE) {
			std::cout << "The model is infeasible; computing IIS..." << std::endl;
			gm.computeIIS();
			gm.write("cluster_model.ilp");
		}
	}
	catch (GRBException &e) {
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
		return;
	}
	catch (...) {
		std::cout << "Unexpected exception during optimization." << std::endl;
		return;
	}
}


int main() {


	return 0;
}


#endif // !MYCPPPITFALLS_LEARNGUROBI_HPP
