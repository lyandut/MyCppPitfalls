//
// @author   liyan
// @contact  lyan_dut@outlook.com
//
#pragma once

#include <list>
#include <set>
#include <numeric>
#include <functional>

#include "Config.hpp"
#include "Instance.hpp"

namespace fbp {

	using namespace std;

	class RandomLocalSearcher {

		/// 排序规则定义
		struct SortRule {
			vector<int> sequence;
			double target_objective;
		};

		/// 自定义set比较函数用于O(1)查找最小宽度和支持删除指定元素
		function<bool(int, int)> rect_comp = [this](int lhs, int rhs) {
			if (_src.at(lhs).width == _src.at(rhs).width) { return lhs < rhs; }
			return _src.at(lhs).width < _src.at(rhs).width;
		};

	public:

		RandomLocalSearcher() = delete;

		RandomLocalSearcher(const Instance &ins, const vector<Rect> &src, int bin_width, default_random_engine &gen) :
			_ins(ins), _src(src), _bin_width(bin_width), _gen(gen), _min_width_rects(rect_comp),
			_uniform_dist(0, _src.size() - 1), _objective(numeric_limits<double>::max()) {
			reset();
			init_sort_rules();
		}

		const vector<Rect>& get_dst() const { return _dst; }

		double get_objective() const { return _objective; }

		int get_area() const { return _obj_area; }

		double get_fill_ratio() const { return _obj_fillratio; }

		double get_wirelength() const { return _obj_wirelength; }

		/// 排版后的高度上界，仅`Config::LevelQAPCluster::On`使用
		int get_bin_height() const {
			return max_element(_skyline.begin(), _skyline.end(), [](auto &lhs, auto &rhs) { return lhs.y < rhs.y; })->y;
		}

		void set_group_boundaries(const vector<Boundary> &boundaries) { _group_boundaries = boundaries; }

		void set_group_neighbors(const vector<vector<bool>> &neighbors) { _group_neighbors = neighbors; }

		/// 基于_bin_width进行随机局部搜索
		void random_local_search(int iter, double alpha, double beta,
			Config::LevelGroupSearch level_gs, Config::LevelWireLength level_wl) {
			// the first time to call RLS on W_k
			if (iter == 1) {
				for (auto &rule : _sort_rules) {
					_rects.assign(rule.sequence.begin(), rule.sequence.end());
					for_each(rule.sequence.begin(), rule.sequence.end(), [this](int rhs) { _min_width_rects.insert(rhs); });

					vector<Rect> target_dst;
					int target_area = insert_bottom_left_score(target_dst, level_gs) * _bin_width;
					double dist;
					double target_wirelength = cal_wirelength(target_dst, dist, level_wl);
					rule.target_objective = cal_objective(target_area, dist, alpha, beta);
					if (rule.target_objective < _objective) {
						_objective = rule.target_objective;
						_obj_area = target_area;
						_obj_fillratio = 1.0 * _ins.get_total_area() / _obj_area;
						_obj_wirelength = target_wirelength;
						_dst = target_dst;
					}
				}
				// 降序排列，越后面的目标函数值越小选中概率越大
				sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_objective > rhs.target_objective; });
			}

			// 迭代优化
			SortRule &picked_rule = _sort_rules[_discrete_dist(_gen)];
			for (int i = 1; i <= iter; ++i) {
				SortRule new_rule = picked_rule;
				if (iter % 4) { swap_sort_rule(new_rule); }
				else { rotate_sort_rule(new_rule); }
				_rects.assign(new_rule.sequence.begin(), new_rule.sequence.end());
				for_each(new_rule.sequence.begin(), new_rule.sequence.end(), [this](int rhs) { _min_width_rects.insert(rhs); });

				vector<Rect> target_dst;
				int target_area = insert_bottom_left_score(target_dst, level_gs) * _bin_width;
				double dist;
				double target_wirelength = cal_wirelength(target_dst, dist, level_wl);
				new_rule.target_objective = cal_objective(target_area, dist, alpha, beta);
				if (new_rule.target_objective < picked_rule.target_objective) {
					picked_rule = new_rule;
					if (picked_rule.target_objective < _objective) {
						_objective = picked_rule.target_objective;
						_obj_area = target_area;
						_obj_fillratio = 1.0 * _ins.get_total_area() / _obj_area;
						_obj_wirelength = target_wirelength;
						_dst = target_dst;
					}
				}
			}
			// 更新排序规则列表
			sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_objective > rhs.target_objective; });
		}

		/// 基于最下最左和打分策略，贪心构造一个完整解
		int insert_bottom_left_score(vector<Rect> &dst, Config::LevelGroupSearch method) {
			int skyline_height = 0;
			reset();
			dst = _src;

			while (!_rects.empty()) {
				auto bottom_skyline_iter = min_element(_skyline.begin(), _skyline.end(),
					[](auto &lhs, auto &rhs) { return lhs.y < rhs.y; });
				int best_skyline_index = distance(_skyline.begin(), bottom_skyline_iter);
				list<int> candidate_rects = get_candidate_rects(best_skyline_index, method);
				int min_rect_width = _src.at(*min_element(candidate_rects.begin(), candidate_rects.end(),
					[this](int lhs, int rhs) { return _src.at(lhs).width < _src.at(rhs).width; })).width;

				if (_skyline[best_skyline_index].width < min_rect_width) { // 最小宽度矩形放不进去，需要填坑
					if (best_skyline_index == 0) { _skyline[best_skyline_index].y = _skyline[best_skyline_index + 1].y; }
					else if (best_skyline_index == _skyline.size() - 1) { _skyline[best_skyline_index].y = _skyline[best_skyline_index - 1].y; }
					else { _skyline[best_skyline_index].y = min(_skyline[best_skyline_index - 1].y, _skyline[best_skyline_index + 1].y); }
					merge_skylines();
					continue;
				}

				int best_rect_index = find_rect_for_skyline_bottom_left(best_skyline_index, candidate_rects, dst);
				assert(best_rect_index != -1);
				assert(_disjoint_rects.add(dst[best_rect_index]));

				// 更新skyline
				SkylineNode new_skyline_node{ dst[best_rect_index].x, dst[best_rect_index].y + dst[best_rect_index].height, dst[best_rect_index].width };
				if (new_skyline_node.x == _skyline[best_skyline_index].x) { // 靠左
					_skyline.insert(_skyline.begin() + best_skyline_index, new_skyline_node);
					_skyline[best_skyline_index + 1].x += new_skyline_node.width;
					_skyline[best_skyline_index + 1].width -= new_skyline_node.width;
					merge_skylines();
				}
				else { // 靠右
					_skyline.insert(_skyline.begin() + best_skyline_index + 1, new_skyline_node);
					_skyline[best_skyline_index].width -= new_skyline_node.width;
					merge_skylines();
				}
				skyline_height = max(skyline_height, new_skyline_node.y);
			}

			return skyline_height;
		}

	private:
		/// 每次迭代重置_skyLine
		void reset() {
			_skyline.clear();
			_skyline.push_back({ 0,0,_bin_width });
			_disjoint_rects.clear();
		}

		/// 初始化排序规则列表
		void init_sort_rules() {
			vector<int> seq(_src.size());
			// 0_输入顺序
			iota(seq.begin(), seq.end(), 0);
			_sort_rules.reserve(5);
			for (int i = 0; i < 5; ++i) { _sort_rules.push_back({ seq, numeric_limits<double>::max() }); }
			// 1_面积递减
			sort(_sort_rules[1].sequence.begin(), _sort_rules[1].sequence.end(), [this](int lhs, int rhs) {
				return  _src.at(lhs).area > _src.at(rhs).area; });
			// 2_高度递减
			sort(_sort_rules[2].sequence.begin(), _sort_rules[2].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs).height > _src.at(rhs).height; });
			// 3_宽度递减
			sort(_sort_rules[3].sequence.begin(), _sort_rules[3].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs).width > _src.at(rhs).width; });
			// 4_随机排序
			shuffle(_sort_rules[4].sequence.begin(), _sort_rules[4].sequence.end(), _gen);

			// 默认输入顺序
			_rects.assign(_sort_rules[0].sequence.begin(), _sort_rules[0].sequence.end());
			for_each(_sort_rules[0].sequence.begin(), _sort_rules[0].sequence.end(), [this](int rhs) { _min_width_rects.insert(rhs); });

			// 离散概率分布初始化
			vector<int> probs; probs.reserve(_sort_rules.size());
			for (int i = 1; i <= _sort_rules.size(); ++i) { probs.push_back(2 * i); }
			_discrete_dist = discrete_distribution<>(probs.begin(), probs.end());
		}

		/// 邻域动作1：交换两个块的顺序
		void swap_sort_rule(SortRule &rule) {
			size_t a = _uniform_dist(_gen);
			size_t b = _uniform_dist(_gen);
			while (a == b) { b = _uniform_dist(_gen); }
			swap(rule.sequence[a], rule.sequence[b]);
		}

		/// 邻域动作2：连续多个块移动
		void rotate_sort_rule(SortRule &rule) {
			size_t a = _uniform_dist(_gen);
			rotate(rule.sequence.begin(), rule.sequence.begin() + a, rule.sequence.end());
		}

		/// 基于分组策略挑选候选矩形，减小搜索规模
		list<int> get_candidate_rects(int skyline_index, Config::LevelGroupSearch method) {
			if (method == Config::LevelGroupSearch::NoGroup) { return _rects; }

			int gid = 0;
			for (; gid < _group_boundaries.size(); ++gid) {
				if (_skyline[skyline_index].x >= _group_boundaries[gid].x
					&& _skyline[skyline_index].y >= _group_boundaries[gid].y
					&& _skyline[skyline_index].x < _group_boundaries[gid].x + _group_boundaries[gid].width
					&& _skyline[skyline_index].y < _group_boundaries[gid].y + _group_boundaries[gid].height) {
					break;
				}
			}
			if (gid == _group_boundaries.size()) { // 超出上边界，从后往前只检查x范围即可
				--gid;
				for (; gid >= 0; --gid) {
					if (_skyline[skyline_index].x >= _group_boundaries[gid].x
						&& _skyline[skyline_index].y >= _group_boundaries[gid].y
						&& _skyline[skyline_index].x < _group_boundaries[gid].x + _group_boundaries[gid].width) {
						break;
					}
				}
			}
			list<int> candidate_rects;
			switch (method) {
			case Config::LevelGroupSearch::NeighborNone:
				for (int r : _rects) { // 必须顺序遍历_rects才能使sort_rule生效
					if (_src.at(r).gid == gid)
						candidate_rects.push_back(r);
				}
				break;
			case Config::LevelGroupSearch::NeighborAll:
				for (int r : _rects) {
					if (_src.at(r).gid == gid)
						candidate_rects.push_back(r);
					else if (_group_neighbors[_src.at(r).gid][gid])
						candidate_rects.push_back(r);
				}
				break;
			case Config::LevelGroupSearch::NeighborPartial:
				for (int r : _rects) {
					if (_src.at(r).gid == gid)
						candidate_rects.push_back(r);
					else if (_group_neighbors[_src.at(r).gid][gid] && _src.at(r).gid < gid)
						candidate_rects.push_back(r);
				}
				break;
			default:
				assert(false);
				break;
			}
			// 如果没有符合策略的矩形，则返回所有矩形
			if (candidate_rects.empty()) { return _rects; }
			return candidate_rects;
		}

		/// 基于左下和打分策略
		int find_rect_for_skyline_bottom_left(int skyline_index, const list<int> &rects, vector<Rect> &dst) {
			int best_rect = -1, best_score = -1;
			for (int r : rects) {
				int x, score;
				for (int rotate = 0; rotate <= 1; ++rotate) {
					int width = dst[r].width, height = dst[r].height;
					if (rotate) { swap(width, height); }
					if (score_rect_for_skyline_bottom_left(skyline_index, width, height, x, score)) {
						if (best_score < score) {
							best_score = score;
							best_rect = r;
							dst[r].x = x;
							dst[r].y = _skyline[skyline_index].y;
							dst[r].width = width;
							dst[r].height = height;
						}
					}
				}
			}
			// (d)(f)(h)->(h)的退化情况
			_min_width_rects.erase(best_rect);
			int min_width_rect = *_min_width_rects.begin();
			int left_width = _skyline[skyline_index].width - dst[best_rect].width;
			if (best_score == 4 && left_width < _src.at(min_width_rect).width) { // (d)导致浪费 ===> 退化成(h)
				_min_width_rects.insert(best_rect); // 恢复
				auto pos = 
				
			}
			if (best_score == 2) {}
			if (best_score == 0) {}

			// 从未放置列表中删除
			_rects.remove(best_rect);
			_min_width_rects.erase(best_rect);

			return best_rect;
		}

		/// Space定义
		struct SkylineSpace {
			int x;
			int y;
			int width;
			int hl;
			int hr;
		};

		SkylineSpace skyline_nodo_to_space(int skyline_index) {
			int hl, hr;
			if (_skyline.size() == 1) {
				hl = hr = INF - _skyline[skyline_index].y;
			}
			else if (skyline_index == 0) {
				hl = INF - _skyline[skyline_index].y;
				hr = _skyline[skyline_index + 1].y - _skyline[skyline_index].y;
			}
			else if (skyline_index == _skyline.size() - 1) {
				hl = _skyline[skyline_index - 1].y - _skyline[skyline_index].y;
				hr = INF - _skyline[skyline_index].y;
			}
			else {
				hl = _skyline[skyline_index - 1].y - _skyline[skyline_index].y;
				hr = _skyline[skyline_index + 1].y - _skyline[skyline_index].y;
			}
			return { _skyline[skyline_index].x, _skyline[skyline_index].y, _skyline[skyline_index].width, hl, hr };
		}

		/// 打分策略
		bool score_rect_for_skyline_bottom_left(int skyline_index, int width, int height, int &x, int &score) {
			if (width > _skyline[skyline_index].width) { return false; }

			SkylineSpace space = skyline_nodo_to_space(skyline_index);
			if (space.hl >= space.hr) {
				if (width == space.width && height == space.hl) { score = 7; }
				else if (width == space.width && height == space.hr) { score = 6; }
				else if (width == space.width && height > space.hl) { score = 5; }
				else if (width < space.width && height == space.hl) { score = 4; }
				else if (width == space.width && height < space.hl && height > space.hr) { score = 3; }
				else if (width < space.width && height == space.hr) { score = 2; } // 靠右
				else if (width == space.width && height < space.hr) { score = 1; }
				else if (width < space.width && height != space.hl) { score = 0; }
				else { return false; }

				if (score == 2) { x = _skyline[skyline_index].x + _skyline[skyline_index].width - width; }
				else { x = _skyline[skyline_index].x; }
			}
			else { // hl < hr
				if (width == space.width && height == space.hr) { score = 7; }
				else if (width == space.width && height == space.hl) { score = 6; }
				else if (width == space.width && height > space.hr) { score = 5; }
				else if (width < space.width && height == space.hr) { score = 4; } // 靠右
				else if (width == space.width && height < space.hr && height > space.hl) { score = 3; }
				else if (width < space.width && height == space.hl) { score = 2; }
				else if (width == space.width && height < space.hl) { score = 1; }
				else if (width < space.width && height != space.hr) { score = 0; } // 靠右
				else { return false; }

				if (score == 4 || score == 0) { x = _skyline[skyline_index].x + _skyline[skyline_index].width - width; }
				else { x = _skyline[skyline_index].x; }
			}
			if (x + width > _bin_width) { return false; }

			return true;
		}

		/// 计算线长，默认引脚在中心
		double cal_wirelength(const vector<Rect> &dst, double &dist, Config::LevelWireLength method) {
			double total_wirelength = 0;
			dist = 0;
			for (auto &net : _ins.get_netlist()) {
				double max_x = 0, min_x = numeric_limits<double>::max();
				double max_y = 0, min_y = numeric_limits<double>::max();
				for (int b : net.block_list) {
					double pin_x = dst.at(b).x + dst.at(b).width * 0.5;
					double pin_y = dst.at(b).y + dst.at(b).height * 0.5;
					max_x = max(max_x, pin_x);
					min_x = min(min_x, pin_x);
					max_y = max(max_y, pin_y);
					min_y = min(min_y, pin_y);
				}
				if (method == Config::LevelWireLength::BlockAndTerminal) {
					for (int t : net.terminal_list) {
						double pad_x = _ins.get_terminals().at(t).x_coordinate;
						double pad_y = _ins.get_terminals().at(t).y_coordinate;
						max_x = max(max_x, pad_x);
						min_x = min(min_x, pad_x);
						max_y = max(max_y, pad_y);
						min_y = min(min_y, pad_y);
					}
				}
				double hpwl = max_x - min_x + max_y - min_y;
				total_wirelength += hpwl;
				dist += hpwl * hpwl;
			}
			return total_wirelength;
		}

		/// 目标函数：EDAthon-2020-P4
		double cal_objective(int area, double dist, double alpha, double beta) {
			return (alpha * area + beta * dist) / _ins.get_total_area();
		}

		/// 合并同一level的skyline节点.
		void merge_skylines() {
			_skyline.erase(
				remove_if(_skyline.begin(), _skyline.end(), [](auto &rhs) { return rhs.width <= 0; }),
				_skyline.end()
			);
			for (int i = 0; i < _skyline.size() - 1; ++i) {
				if (_skyline[i].y == _skyline[i + 1].y) {
					_skyline[i].width += _skyline[i + 1].width;
					_skyline.erase(_skyline.begin() + i + 1);
					--i;
				}
			}
		}

	private:
		const Instance &_ins;
		const vector<Rect> &_src;
		const int _bin_width;
		default_random_engine &_gen;

		// 优化目标
		vector<Rect> _dst;
		double _objective;
		int _obj_area;
		double _obj_fillratio;
		double _obj_wirelength;

		Skyline _skyline;

		list<int> _rects; // SortRule的sequence，相当于指针，使用list快速删除，放置完毕为空
		set<int, decltype(rect_comp)> _min_width_rects; // 相当于可以删除指定元素的优先队列

		// 排序规则列表，用于随机局部搜索
		vector<SortRule> _sort_rules;
		discrete_distribution<> _discrete_dist;   // 离散概率分布，用于挑选规则(即挑选sequence赋给_rects)
		uniform_int_distribution<> _uniform_dist; // 均匀分布，用于交换矩形顺序

		// 分组信息，用于`get_candidate_rects()`挑选候选矩形
		vector<Boundary> _group_boundaries;    // `_group_boundaries[i]`   分组_i的边界
		vector<vector<bool>> _group_neighbors; // `_group_neighbors[i][j]` 分组_i和_j是否为邻居

		// 重叠检测，仅debug
		DisjointRects _disjoint_rects;
	};
}
