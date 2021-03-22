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

		/// ���������
		struct SortRule {
			vector<int> sequence;
			double target_objective;
		};

		/// �Զ���set�ȽϺ�������O(1)������С��Ⱥ�֧��ɾ��ָ��Ԫ��
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

		/// �Ű��ĸ߶��Ͻ磬��`Config::LevelQAPCluster::On`ʹ��
		int get_bin_height() const {
			return max_element(_skyline.begin(), _skyline.end(), [](auto &lhs, auto &rhs) { return lhs.y < rhs.y; })->y;
		}

		void set_group_boundaries(const vector<Boundary> &boundaries) { _group_boundaries = boundaries; }

		void set_group_neighbors(const vector<vector<bool>> &neighbors) { _group_neighbors = neighbors; }

		/// ����_bin_width��������ֲ�����
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
				// �������У�Խ�����Ŀ�꺯��ֵԽСѡ�и���Խ��
				sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_objective > rhs.target_objective; });
			}

			// �����Ż�
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
			// ������������б�
			sort(_sort_rules.begin(), _sort_rules.end(), [](auto &lhs, auto &rhs) { return lhs.target_objective > rhs.target_objective; });
		}

		/// ������������ʹ�ֲ��ԣ�̰�Ĺ���һ��������
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

				if (_skyline[best_skyline_index].width < min_rect_width) { // ��С��Ⱦ��ηŲ���ȥ����Ҫ���
					if (best_skyline_index == 0) { _skyline[best_skyline_index].y = _skyline[best_skyline_index + 1].y; }
					else if (best_skyline_index == _skyline.size() - 1) { _skyline[best_skyline_index].y = _skyline[best_skyline_index - 1].y; }
					else { _skyline[best_skyline_index].y = min(_skyline[best_skyline_index - 1].y, _skyline[best_skyline_index + 1].y); }
					merge_skylines();
					continue;
				}

				int best_rect_index = find_rect_for_skyline_bottom_left(best_skyline_index, candidate_rects, dst);
				assert(best_rect_index != -1);
				assert(_disjoint_rects.add(dst[best_rect_index]));

				// ����skyline
				SkylineNode new_skyline_node{ dst[best_rect_index].x, dst[best_rect_index].y + dst[best_rect_index].height, dst[best_rect_index].width };
				if (new_skyline_node.x == _skyline[best_skyline_index].x) { // ����
					_skyline.insert(_skyline.begin() + best_skyline_index, new_skyline_node);
					_skyline[best_skyline_index + 1].x += new_skyline_node.width;
					_skyline[best_skyline_index + 1].width -= new_skyline_node.width;
					merge_skylines();
				}
				else { // ����
					_skyline.insert(_skyline.begin() + best_skyline_index + 1, new_skyline_node);
					_skyline[best_skyline_index].width -= new_skyline_node.width;
					merge_skylines();
				}
				skyline_height = max(skyline_height, new_skyline_node.y);
			}

			return skyline_height;
		}

	private:
		/// ÿ�ε�������_skyLine
		void reset() {
			_skyline.clear();
			_skyline.push_back({ 0,0,_bin_width });
			_disjoint_rects.clear();
		}

		/// ��ʼ����������б�
		void init_sort_rules() {
			vector<int> seq(_src.size());
			// 0_����˳��
			iota(seq.begin(), seq.end(), 0);
			_sort_rules.reserve(5);
			for (int i = 0; i < 5; ++i) { _sort_rules.push_back({ seq, numeric_limits<double>::max() }); }
			// 1_����ݼ�
			sort(_sort_rules[1].sequence.begin(), _sort_rules[1].sequence.end(), [this](int lhs, int rhs) {
				return  _src.at(lhs).area > _src.at(rhs).area; });
			// 2_�߶ȵݼ�
			sort(_sort_rules[2].sequence.begin(), _sort_rules[2].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs).height > _src.at(rhs).height; });
			// 3_��ȵݼ�
			sort(_sort_rules[3].sequence.begin(), _sort_rules[3].sequence.end(), [this](int lhs, int rhs) {
				return _src.at(lhs).width > _src.at(rhs).width; });
			// 4_�������
			shuffle(_sort_rules[4].sequence.begin(), _sort_rules[4].sequence.end(), _gen);

			// Ĭ������˳��
			_rects.assign(_sort_rules[0].sequence.begin(), _sort_rules[0].sequence.end());
			for_each(_sort_rules[0].sequence.begin(), _sort_rules[0].sequence.end(), [this](int rhs) { _min_width_rects.insert(rhs); });

			// ��ɢ���ʷֲ���ʼ��
			vector<int> probs; probs.reserve(_sort_rules.size());
			for (int i = 1; i <= _sort_rules.size(); ++i) { probs.push_back(2 * i); }
			_discrete_dist = discrete_distribution<>(probs.begin(), probs.end());
		}

		/// ������1�������������˳��
		void swap_sort_rule(SortRule &rule) {
			size_t a = _uniform_dist(_gen);
			size_t b = _uniform_dist(_gen);
			while (a == b) { b = _uniform_dist(_gen); }
			swap(rule.sequence[a], rule.sequence[b]);
		}

		/// ������2������������ƶ�
		void rotate_sort_rule(SortRule &rule) {
			size_t a = _uniform_dist(_gen);
			rotate(rule.sequence.begin(), rule.sequence.begin() + a, rule.sequence.end());
		}

		/// ���ڷ��������ѡ��ѡ���Σ���С������ģ
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
			if (gid == _group_boundaries.size()) { // �����ϱ߽磬�Ӻ���ǰֻ���x��Χ����
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
				for (int r : _rects) { // ����˳�����_rects����ʹsort_rule��Ч
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
			// ���û�з��ϲ��Եľ��Σ��򷵻����о���
			if (candidate_rects.empty()) { return _rects; }
			return candidate_rects;
		}

		/// �������ºʹ�ֲ���
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
			// (d)(f)(h)->(h)���˻����
			_min_width_rects.erase(best_rect);
			int min_width_rect = *_min_width_rects.begin();
			int left_width = _skyline[skyline_index].width - dst[best_rect].width;
			if (best_score == 4 && left_width < _src.at(min_width_rect).width) { // (d)�����˷� ===> �˻���(h)
				_min_width_rects.insert(best_rect); // �ָ�
				auto pos = 
				
			}
			if (best_score == 2) {}
			if (best_score == 0) {}

			// ��δ�����б���ɾ��
			_rects.remove(best_rect);
			_min_width_rects.erase(best_rect);

			return best_rect;
		}

		/// Space����
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

		/// ��ֲ���
		bool score_rect_for_skyline_bottom_left(int skyline_index, int width, int height, int &x, int &score) {
			if (width > _skyline[skyline_index].width) { return false; }

			SkylineSpace space = skyline_nodo_to_space(skyline_index);
			if (space.hl >= space.hr) {
				if (width == space.width && height == space.hl) { score = 7; }
				else if (width == space.width && height == space.hr) { score = 6; }
				else if (width == space.width && height > space.hl) { score = 5; }
				else if (width < space.width && height == space.hl) { score = 4; }
				else if (width == space.width && height < space.hl && height > space.hr) { score = 3; }
				else if (width < space.width && height == space.hr) { score = 2; } // ����
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
				else if (width < space.width && height == space.hr) { score = 4; } // ����
				else if (width == space.width && height < space.hr && height > space.hl) { score = 3; }
				else if (width < space.width && height == space.hl) { score = 2; }
				else if (width == space.width && height < space.hl) { score = 1; }
				else if (width < space.width && height != space.hr) { score = 0; } // ����
				else { return false; }

				if (score == 4 || score == 0) { x = _skyline[skyline_index].x + _skyline[skyline_index].width - width; }
				else { x = _skyline[skyline_index].x; }
			}
			if (x + width > _bin_width) { return false; }

			return true;
		}

		/// �����߳���Ĭ������������
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

		/// Ŀ�꺯����EDAthon-2020-P4
		double cal_objective(int area, double dist, double alpha, double beta) {
			return (alpha * area + beta * dist) / _ins.get_total_area();
		}

		/// �ϲ�ͬһlevel��skyline�ڵ�.
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

		// �Ż�Ŀ��
		vector<Rect> _dst;
		double _objective;
		int _obj_area;
		double _obj_fillratio;
		double _obj_wirelength;

		Skyline _skyline;

		list<int> _rects; // SortRule��sequence���൱��ָ�룬ʹ��list����ɾ�����������Ϊ��
		set<int, decltype(rect_comp)> _min_width_rects; // �൱�ڿ���ɾ��ָ��Ԫ�ص����ȶ���

		// ��������б���������ֲ�����
		vector<SortRule> _sort_rules;
		discrete_distribution<> _discrete_dist;   // ��ɢ���ʷֲ���������ѡ����(����ѡsequence����_rects)
		uniform_int_distribution<> _uniform_dist; // ���ȷֲ������ڽ�������˳��

		// ������Ϣ������`get_candidate_rects()`��ѡ��ѡ����
		vector<Boundary> _group_boundaries;    // `_group_boundaries[i]`   ����_i�ı߽�
		vector<vector<bool>> _group_neighbors; // `_group_neighbors[i][j]` ����_i��_j�Ƿ�Ϊ�ھ�

		// �ص���⣬��debug
		DisjointRects _disjoint_rects;
	};
}
