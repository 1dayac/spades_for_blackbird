/*
 * path_utils.hpp
 *
 *  Created on: Aug 17, 2011
 *      Author: andrey
 */

#ifndef PATH_UTILS_HPP_
#define PATH_UTILS_HPP_

#include "lc_common.hpp"
#include "extend.hpp"

namespace long_contigs {

//Recounting lengths form all edges to path's end
void RecountLengthsForward(Graph& g, BidirectionalPath& path, PathLengths& lengths) {
	lengths.clear();
	double currentLength = 0;

	for(auto iter = path.rbegin(); iter != path.rend(); ++iter) {
		currentLength += g.length(*iter);
		lengths.push_front(currentLength);
	}
}

//Recounting lengths from path's start to all edges
void RecountLengthsBackward(Graph& g, BidirectionalPath& path, PathLengths& lengths) {
	lengths.clear();
	double currentLength = 0;

	for(auto iter = path.begin(); iter != path.end(); ++iter) {
		lengths.push_back(currentLength);
		currentLength += g.length(*iter);
	}
}

bool ComparePaths(const BidirectionalPath& path1, const BidirectionalPath& path2) {
	if (path1.size() != path2.size()) {
		return false;
	}

	for (size_t i = 0; i < path1.size(); ++i) {
		if (path1[i] != path2[i]) {
			return false;
		}
	}
	return true;
}

template<class T>
bool ContainsPath(const T& path, const EdgeId sample) {
	for (size_t i = 0; i < path.size(); ++i) {
		if (sample == path[i]) {
			return true;
		}
	}
	return false;
}

bool ContainsPath(const BidirectionalPath& path, const BidirectionalPath& sample) {
	if (path.size() < sample.size()) {
		return false;
	}

	for (size_t i = 0; i < path.size() - sample.size() + 1 ; ++i) {
		bool found = true;

		for (size_t j = 0; j < sample.size(); ++j) {
			if (sample[j] != path[i + j]) {
				found = false;
				break;
			}
		}

		if (found) {
			return true;
		}
	}
	return false;
}

bool ComplementPaths(Graph& g, const BidirectionalPath& path1, const BidirectionalPath& path2) {
	if (path1.size() != path2.size()) {
		return false;
	}

	for (size_t i = 0; i < path1.size(); ++i) {
		if (path1[i] != g.conjugate(path2[path1.size() - i - 1])) {
			return false;
		}
	}

	return true;
}

bool ContainsComplementPath(Graph& g, const BidirectionalPath& path, const BidirectionalPath& sample) {
	if (path.size() < sample.size()) {
		return false;
	}

	for (size_t i = 0; i < path.size() - sample.size() + 1 ; ++i) {
		bool found = true;

		for (size_t j = 0; j < sample.size(); ++j) {
			if (g.conjugate(sample[sample.size() - j - 1]) != path[i + j]) {
				found = false;
				break;
			}
		}

		if (found) {
			return true;
		}
	}
	return false;
}

size_t ContainsCommonComplementPath(Graph& g, const BidirectionalPath& path1, const BidirectionalPath& path2, int* start) {
	size_t length;
	size_t maxLen = 0;
	*start = 0;

	for (int i = - (int) path2.size() + 1; i < (int) path1.size(); ++i) {
		length = 0;

		for (int j = 0; j < (int) path2.size() && i + j < (int) path1.size(); ++j) {
			if (i + j < 0) {
				continue;
			}
			if (g.conjugate(path2[path2.size() - j - 1]) != path1[i + j]) {
				length = 0;
				break;
			}
			length += g.length(path1[i + j]);
		}

		if (length > maxLen) {
			*start = i;
			maxLen = length;
		}
	}
	return maxLen;
}

size_t LongestCommonComplement(Graph& g, const BidirectionalPath& path1, const BidirectionalPath& path2, int * start1, int * start2, int * edges) {
	size_t length;
	size_t maxLen = 0;
	int curEdges;
	*edges = 0;

	for (int i = - (int) path2.size() + 1; i < (int) path1.size(); ++i) {
		length = 0;
		curEdges = 0;
		int j = 0;

		for (; j < (int) path2.size() && i + j < (int) path1.size(); ++j) {
			if (i + j < 0) {
				continue;
			}
			if (g.conjugate(path2[path2.size() - j - 1]) != path1[i + j]) {
				if (length > maxLen) {
					*start1 = i + j - curEdges;
					*start2 = path2.size() - j;
					*edges = curEdges;
					maxLen = length;
				}
				length = 0;
				curEdges = 0;
			} else {
				++curEdges;
				length += g.length(path1[i + j]);
			}
		}

		if (length > maxLen) {
			*start1 = i + j - curEdges;
			*start2 = path2.size() - j;
			*edges = curEdges;
			maxLen = length;
		}
	}
	return maxLen;
}

void GetSubpath(const BidirectionalPath& path, BidirectionalPath * res, int from, int n = -1) {
	res->clear();
	int to = (n < 0) ? path.size() : from + n;
	for (int i = from; i < to; ++i) {
		res->push_back(path[i]);
	}
}

void SetCorrectIds(std::vector<BidirectionalPath>& paths, int i) {
	paths[i].id = i;
	paths[i].conj_id = i + 1;
	paths[i + 1].id = i + 1;
	paths[i + 1].conj_id = i;

}

void RecountIds(Graph& g, std::vector<BidirectionalPath>& paths) {
	for (int i = 0; i < (int) paths.size(); i += 2) {
		if (paths[i].id == paths[i+1].conj_id && paths[i+1].id == paths[i].conj_id) {
			SetCorrectIds(paths, i);
		} else {
			INFO("Pair of paths seem to be not conjugate, wrong ids detected: " << paths[i].id << ", " << paths[i+1].conj_id  << ", " << paths[i+1].id  << ", " << paths[i].conj_id);
			DetailedPrintPath(g, paths[i]);
			DetailedPrintPath(g, paths[paths[i].conj_id]);
			DetailedPrintPath(g, paths[i+1]);
			DetailedPrintPath(g, paths[paths[i+1].conj_id]);
		}
	}
}

void CheckIds(Graph& g, std::vector<BidirectionalPath>& paths) {
	INFO("Checking IDS");
	for (int i = 0; i < (int) paths.size(); i += 2) {
		if (paths[i].id != paths[i+1].conj_id || paths[i+1].id != paths[i].conj_id
				|| paths[i].id != i || paths[i+1].id != i+1) {
			INFO("Pair of paths seem to be not conjugate, wrong ids detected: " << paths[i].id << ", " << paths[i+1].conj_id  << ", " << paths[i+1].id  << ", " << paths[i].conj_id);
			DetailedPrintPath(g, paths[i]);
			DetailedPrintPath(g, paths[paths[i].conj_id]);
			DetailedPrintPath(g, paths[i+1]);
			DetailedPrintPath(g, paths[paths[i+1].conj_id]);
		}
	}
}

void SortPathsByLength(Graph& g, std::vector<BidirectionalPath>& paths) {
	SimplePathComparator pathComparator(g);
	std::stable_sort(paths.begin(), paths.end(), pathComparator);
	RecountIds(g, paths);
}

void FindAllComplementParts(Graph& g, const BidirectionalPath& path1, const BidirectionalPath& path2, size_t minConjLen, std::vector<BidirectionalPath> * newPaths) {
	newPaths->clear();
	std::deque<BidirectionalPath> queue;
	queue.push_back(BidirectionalPath(path1));
	queue.push_back(BidirectionalPath(path2));

	while(!queue.empty()) {
		int s1, s2;
		int edges;
		if (LongestCommonComplement(g, queue[0], queue[1], &s1, &s2, &edges) >= minConjLen) {
			BidirectionalPath p1;
			GetSubpath(queue[0], &p1, s1, edges);
			BidirectionalPath p2;
			GetSubpath(queue[1], &p2, s2, edges);
			newPaths->push_back(p1);
			newPaths->push_back(p2);

			INFO("Found common path: " << s1 << ", " << s2 << ", " << edges);
			DetailedPrintPath(g, p1);
			DetailedPrintPath(g, p2);

			BidirectionalPath l1;
			GetSubpath(queue[0], &l1, s1 + edges);
			BidirectionalPath l2;
			GetSubpath(queue[1], &l2, 0 , s2);

			INFO("Left parts");
			DetailedPrintPath(g, l1);
			DetailedPrintPath(g, l2);

			if (!l1.empty() && !l2.empty()) {
				queue.push_back(l1);
				queue.push_back(l2);
			}

			BidirectionalPath r1;
			GetSubpath(queue[0], &r1, 0, s1);
			BidirectionalPath r2;
			GetSubpath(queue[1], &r2, s2 + edges);

			INFO("Right parts");
			DetailedPrintPath(g, r1);
			DetailedPrintPath(g, r2);

			if (!r1.empty() && !r2.empty()) {
				queue.push_back(r1);
				queue.push_back(r2);
			}
		}
		queue.pop_front();
		queue.pop_front();
	}
}

size_t LengthComplement(Graph& g, const BidirectionalPath& path, const BidirectionalPath& sample) {
	if (path.size() < sample.size()) {
		return false;
	}

	size_t max = 0;
	for (size_t i = 0; i < path.size() - sample.size() + 1 ; ++i) {
		size_t found = 0;

		for (size_t j = 0; j < sample.size(); ++j) {
			if (g.conjugate(sample[sample.size() - j - 1]) != path[i + j]) {
				continue;
			}
			found += g.length(path[i + j]);
		}

		if (max < found) {
			max = found;
		}
	}
	return max;
}

template<class T>
bool ContainsAnyOf(const BidirectionalPath& path, T& pathCollection) {
	for (auto iter = pathCollection.begin(); iter != pathCollection.end(); ++iter) {
		if (ContainsPath(path, *iter)) {
			return true;
		}
	}
	return false;
}

bool ContainsPathAt(const BidirectionalPath& path, const EdgeId sample, size_t at = 0) {
	if (at >= path.size()) {
		return false;
	}

	return path[at] == sample;
}

bool ContainsPathAt(const BidirectionalPath& path, const BidirectionalPath& sample, size_t at = 0) {
	if (path.size() < at + sample.size()) {
		return false;
	}

	for (size_t j = 0; j < sample.size(); ++j) {
		if (sample[j] != path[at + j]) {
			return false;
		}
	}

	return true;
}

template<class T>
bool ContainsAnyAt(const BidirectionalPath& path, T& pathCollection, size_t at = 0) {
	for (auto iter = pathCollection.begin(); iter != pathCollection.end(); ++iter) {
		if (ContainsPathAt(path, *iter, at)) {
			return true;
		}
	}
	return false;
}

//Find coverage of worst covered edge
double PathMinReadCoverage(Graph& g, BidirectionalPath& path) {
	if (path.empty()) {
		return 0;
	}

	double minCov = g.coverage(path[0]);

	for (auto edge = path.begin(); edge != path.end(); ++edge) {
		double cov = g.coverage(*edge);
		if (minCov > cov) {
			minCov = cov;
		}
	}

	return minCov;
}

//Filter paths by containing paths
void FilterPaths(Graph& g, std::vector<BidirectionalPath>& paths, std::vector<BidirectionalPath>& samples) {
	for(auto path = paths.begin(); path != paths.end(); ) {
		bool toErase = true;
		for (auto sample = samples.begin(); sample != samples.end(); ++sample) {
			if (ContainsPath(*path, *sample)) {
				toErase = false;
				break;
			}
		}
		if (toErase) {
			paths.erase(path);
		} else {
			++path;
		}
	}
}

void FilterComlementEdges(Graph& g, std::set<EdgeId>& filtered, std::set<EdgeId>& rest) {
	size_t edges = 0;
	for (auto iter = g.SmartEdgeBegin(); !iter.IsEnd(); ++iter) {
		++edges;
		if (rest.count(*iter) == 0) {
			filtered.insert(*iter);
			if (g.conjugate(*iter) != *iter) {
				rest.insert(g.conjugate(*iter));
			}
		}
	}
	INFO("Edges separated by " << filtered.size() << " and " << rest.size() << " from " << edges);
}

void FilterComlementEdges(Graph& g, std::set<EdgeId>& filtered) {
	std::set<EdgeId> rest;
	FilterComlementEdges(g, filtered, rest);
}

//Filter paths only with edges with given length
void FilterEdge(Graph& g, std::vector<BidirectionalPath>& paths, size_t edgeLen) {
	std::vector<BidirectionalPath> samples;
	for (auto edge = g.SmartEdgeBegin(); !edge.IsEnd(); ++edge) {
		if (g.length(*edge) == edgeLen) {
			BidirectionalPath sample;
			sample.push_back(*edge);
			samples.push_back(sample);
		}
	}
	FilterPaths(g, paths, samples);
}

//Remove paths with low covered edges
void FilterLowCovered(Graph& g, std::vector<BidirectionalPath>& paths, double threshold,
		std::vector<BidirectionalPath> * output) {

	output->clear();
	for (int i = 0; i < (int) paths.size(); i += 2) {
		if (math::ge(PathMinReadCoverage(g, paths[i]), threshold)) {
			AddPathPairToContainer(paths[i], paths[i+1], *output);
		}
	}
}



//Remove duplicate paths
void RemoveDuplicate(Graph& g, const std::vector<BidirectionalPath>& paths,
		std::vector<BidirectionalPath>& output,
		std::vector<double>* quality = 0) {

	std::vector<BidirectionalPath> temp(paths.size());
	std::copy(paths.begin(), paths.end(), temp.begin());

	SortPathsByLength(g, temp);

	output.clear();
	if (quality != 0) {
		quality->clear();
	}

	for (int i = 0; i < (int) temp.size(); i += 2) {
		bool copy = true;
		for (int j = 0; j < (int) output.size(); ++j) {
			if (ComparePaths(output[j], temp[i])) {
				copy = false;
				if (quality != 0) {
					quality->at(j) += 1.0;
					quality->at(output[j].conj_id) += 1.0;
				}
				break;
			}
		}

		if (copy) {
			AddPathPairToContainer(temp[i], temp[temp[i].conj_id], output);
			if (quality != 0) {
				quality->push_back(1.0);
				quality->push_back(1.0);
			}
		}
	}
}

//Remove subpaths
void RemoveSubpaths(Graph& g, std::vector<BidirectionalPath>& paths,
		std::vector<BidirectionalPath>& output,
		std::vector<double>* quality = 0) {

	std::vector<BidirectionalPath> temp(paths.size());
	std::copy(paths.begin(), paths.end(), temp.begin());
	SortPathsByLength(g, temp);

	std::vector<size_t> lengths;
	CountPathLengths(g, temp, lengths);

	output.clear();
	if (quality != 0) {
		quality->clear();
	}

	for (int i = 0; i < (int) temp.size(); i += 2) {
		bool copy = true;
		for (int j = 0; j < (int) output.size(); ++j) {
			if (ContainsPath(output[j], temp[i])) {
				copy = false;
				if (quality != 0) {
					double q = ((double) lengths[i]) / ((double) lengths[j]);
					quality->at(j) += q;
					quality->at(output[j].conj_id) += q;
				}
				break;
			}
		}

		if (copy) {
			AddPathPairToContainer(temp[i], temp[temp[i].conj_id], output);
			if (quality != 0) {
				quality->push_back(1.0);
				quality->push_back(1.0);
			}
		}
	}
}

typedef std::multiset<EdgeId> EdgeStat;

void CountSimilarity(Graph& g, EdgeStat& path1, EdgeStat& path2, int * similarEdges, int * similarLen) {
	similarEdges = 0;
	similarLen = 0;

	auto iter = path1.begin();
	while (iter != path1.end()) {
		int count = std::min(path1.count(*iter), path2.count(*iter));
		similarEdges += count;
		similarLen += count * g.length(*iter);
		EdgeId current = *iter;
		while (iter != path1.end() && current == *iter) {
			++iter;
		}
	}

}

void CountStat(BidirectionalPath& path, EdgeStat * stat) {
	stat->clear();
	for (int j = 0; j < (int) path.size(); ++j) {
		stat->insert(path[j]);
	}
}

//Remove similar paths
void RemoveSimilar(Graph& g, std::vector<BidirectionalPath>& paths,
		std::vector<double>& quality,
		std::vector<BidirectionalPath> * output) {

	INFO("Removing similar");

	std::vector<BidirectionalPath> temp(paths.size());
	std::copy(paths.begin(), paths.end(), temp.begin());
	SortPathsByLength(g, temp);

	output->clear();
	output->reserve(temp.size());
	std::vector<EdgeStat> pathStat;
	pathStat.reserve(temp.size());
	std::vector<size_t> lengths;
	lengths.reserve(temp.size());

	for (int i = 0; i < (int) temp.size(); i += 2) {
		bool copy = true;
		EdgeStat stat;
		CountStat(temp[i], &stat);
		size_t length = PathLength(g, temp[i]);

		for (int j = 0; j < (int) output->size(); ++j) {
			int similarLen = 0;
			int similarEdges = 0;

			CountSimilarity(g, stat, pathStat[j], &similarEdges, &similarLen);

			if (math::ge(((double) similarLen) / ((double) length), lc_cfg::get().fo.similar_length) &&
					math::ge(((double) similarEdges) / ((double) temp[i].size()), lc_cfg::get().fo.similar_edges)) {

				copy = false;
				break;
			}
		}

		if (copy) {
			AddPathPairToContainer(temp[i], temp[i + 1], *output);

			pathStat.push_back(stat);
			EdgeStat conjStat;
			CountStat(temp[i + 1], &conjStat);
			pathStat.push_back(conjStat);

			lengths.push_back(length);
			lengths.push_back(length);
		}
	}

	INFO("Done");
}


bool HasConjugate(Graph& g, BidirectionalPath& path) {
	size_t count = 0;
	double len = 0.0;

	for (auto e1 = path.begin(); e1 != path.end(); ++e1) {
		for (auto e2 = path.begin(); e2 != path.end(); ++e2) {
			if (*e1 != *e2 && g.conjugate(*e1) == *e2) {
				++count;
				len += g.length(*e2);
				break;
			}
		}
	}

	if (count != 0) {
		DETAILED_INFO("Self conjugate detected: edges " << count << ", length: " << len);
		DetailedPrintPath(g, path);
	}

	return math::gr(len / PathLength(g, path), 0.0);
}


void BreakApart(Graph& g, std::vector<BidirectionalPath>& paths, int index, std::vector<BidirectionalPath> * output) {
	BidirectionalPath& path = paths[index];
	int i, j;
	bool found = false;

	for (i = 0; i < (int) path.size(); ++i) {
		for (j = path.size() - 1; j > i; --j) {
			if (g.conjugate(path[i]) == path[j]) {
				found = true;
				break;
			}
		}
		if (found) {
			break;
		}
	}

	if (!found) {
		return;
	}

	while (g.conjugate(path[i]) == path[j]) {
		++i;
		--j;
	}

	for (int k = i; k <= j; ++k) {
		if (g.length(path[k]) >= K - lc_cfg::get().fo.chimeric_delta &&
				g.length(path[k]) <= K + lc_cfg::get().fo.chimeric_delta) {

			BidirectionalPath left;
			GetSubpath(path, &left, 0, k);
			BidirectionalPath cleft;
			GetSubpath(paths[path.conj_id], &cleft, path.size() - k);

			if (PathLength(g, left) >= lc_cfg::get().fo.conj_len_percent) {
				DETAILED_INFO("Following parts are added:");
				AddPathPairToContainer(left, cleft, *output);
			}

			DETAILED_INFO("Left of chimeric edge");
			DetailedPrintPath(g, left);
			DetailedPrintPath(g, cleft);

			BidirectionalPath right;
			GetSubpath(path, &right, k + 1);
			BidirectionalPath cright;
			GetSubpath(paths[path.conj_id], &cright, 0, path.size() - k - 1);

			if (PathLength(g, right) >= lc_cfg::get().fo.conj_len_percent) {
				DETAILED_INFO("Following parts are added:");
				AddPathPairToContainer(right, cright, *output);
			}

			DETAILED_INFO("Right of chimeric edge");
			DetailedPrintPath(g, right);
			DetailedPrintPath(g, cright);

			return;
		}
	}

	BidirectionalPath left;
	GetSubpath(path, &left, 0, i);
	BidirectionalPath cleft;
	GetSubpath(paths[path.conj_id], &cleft, path.size() - i);
	if (PathLength(g, left) >= lc_cfg::get().fo.conj_len_percent) {
		DETAILED_INFO("Following parts are added:");
		AddPathPairToContainer(left, cleft, *output);
	}

	DETAILED_INFO("Left part");
	DetailedPrintPath(g, left);
	DetailedPrintPath(g, cleft);

	BidirectionalPath right;
	GetSubpath(path, &right, j + 1);
	BidirectionalPath cright;
	GetSubpath(paths[path.conj_id], &cright, 0, path.size() - j - 1);

	if (PathLength(g, right) >= lc_cfg::get().fo.conj_len_percent) {
		DETAILED_INFO("Following parts are added:");
		AddPathPairToContainer(right, cright, *output);
	}

	DETAILED_INFO("Right part");
	DetailedPrintPath(g, right);
	DetailedPrintPath(g, cright);
}

void RemoveWrongConjugatePaths(Graph& g, std::vector<BidirectionalPath>& paths,
		std::vector<BidirectionalPath> * output) {

	output->clear();
	for (int i = 0; i < (int) paths.size(); i += 2) {
		if (!HasConjugate(g, paths[i]) && !HasConjugate(g, paths[i + 1])) {
			AddPathPairToContainer(paths[i], paths[i+1], *output);
		} else {
			INFO("Removed as self conjugate");
			if (lc_cfg::get().fo.break_sc) {
				INFO("Added half");
				BreakApart(g, paths, i, output);
			}
		}
	}
}


std::pair<int, int> FindSameSearchRange(std::vector<BidirectionalPath>& paths, std::vector<size_t>& lengths, int pathNum) {
	size_t length = lengths[pathNum];

	int i = pathNum - 1;
	while (i >= 0 && lengths[i] == length) {
		--i;
	}

	int j = pathNum + 1;
	while (j < (int) paths.size() && lengths[j] == length) {
		++j;
	}

	return std::make_pair(i + 1, j - 1);
}


std::pair<int, int> FindSearchRange(std::vector<BidirectionalPath>& paths, std::vector<size_t>& lengths, int pathNum) {
	static double coeff = lc_cfg::get().fo.length_percent;
	double length = (double) lengths[pathNum];

	int i = pathNum - 1;
	while (i >=0 && lengths[i] <= length * coeff) {
		--i;
	}

	int j = pathNum + 1;
	while (j < (int) paths.size() && lengths[j] >= length / coeff) {
		++j;
	}

	return std::make_pair(i + 1, j - 1);
}


std::pair<int, double> FindComlementPath(Graph& g, std::vector<BidirectionalPath>& paths, std::vector<size_t>& lengths, int pathNum) {
	static double conjugatePercent = lc_cfg::get().fo.conjugate_percent;
	BidirectionalPath& path = paths[pathNum];

	auto range = FindSameSearchRange(paths, lengths, pathNum);
	for (int i = range.first; i <= range.second; ++i) {
		if (ComplementPaths(g, path, paths[i])) {
			INFO("Total complemented");
			return std::make_pair(i, 1.0);
		}
	}

	double maxConj = 0;
	int maxI = 0;

	range = FindSearchRange(paths, lengths, pathNum);
	for (int i = range.first; i <= range.second; ++i) {
		BidirectionalPath& p = (path.size() < paths[i].size()) ? paths[i] : path;
		BidirectionalPath& sample = (path.size() < paths[i].size()) ? path : paths[i];

		if (ContainsComplementPath(g, p, sample)) {
			INFO("Complement subpath " << ((double) PathLength(g, sample)) / ((double) PathLength(g, p)));
			PrintPath(g, p);
			PrintPath(g, sample);
			return std::make_pair(i, ((double) PathLength(g, sample)) / ((double) PathLength(g, p)));
		}

		size_t conjLength = LengthComplement(g, p, sample);
		double c = ((double) conjLength) / ((double) PathLength(g, p));

		if (c > maxConj) {
			maxConj = c;
			maxI = i;
		}
	}

	if (maxConj >= conjugatePercent) {
		INFO("Partly complement with " << maxI << ", percentage " << maxConj);
		PrintPath(g, path);
		PrintPath(g, paths[maxI]);
		return std::make_pair(maxI, maxConj);
	}
	else {
		INFO("NO COMPLEMENT!");
		PrintPath(g, path);
		return std::make_pair(-1, 0);
	}
}

//Filter symetric complement contigs
void FilterComplement(Graph& g, std::vector<BidirectionalPath>& paths, std::vector<int>* pairs, std::vector<double>* quality) {

	SortPathsByLength(g, paths);
	pairs->clear();
	quality->clear();
	pairs->resize(paths.size(), -1);
	quality->resize(paths.size(), 0);

	std::vector<size_t> lengths;
	CountPathLengths(g, paths, lengths);

	std::set<int> found;

	int i = 0;
	int revert = -1;
	while (i < (int) paths.size()) {
		if (found.count(i) == 0) {
			auto comp = FindComlementPath(g, paths, lengths, i);

			if (found.count(comp.first) > 0 && comp.first != i && comp.first != -1) {
				INFO("Wrong complement pairing");
				PrintPath(g, paths[i]);
				PrintPath(g, paths[pairs->at(comp.first)]);
				PrintPath(g, paths[comp.first]);
				INFO("Substituting");

				if (math::le(comp.second, quality->at(comp.first))) {
					++i;
					continue;
				}
				INFO("Will substitute. New quality " << comp.second << " greater than " << quality->at(comp.first));
				found.erase(pairs->at(comp.first));
				pairs->at(pairs->at(comp.first)) = -1;
				quality->at(pairs->at(comp.first)) = 0;
				revert = pairs->at(comp.first);
			}

			if (comp.first == -1) {
				INFO("Really not found");
				++i;
				continue;
			}

			found.insert(i);
			found.insert(comp.first);
			pairs->at(i) = comp.first;
			pairs->at(comp.first) = i;
			quality->at(i) = comp.second;
			quality->at(comp.first) = comp.second;
		}
		++i;

		if (revert != -1 && i > revert) {
			INFO("Reverting from " << i << " to " << revert);
			i = revert;
			revert = -1;
		}
	}

	INFO("Results of complement filtering")
	found.clear();
	for (int i = 0; i < (int) paths.size(); ++i) {
		if (found.count(i) == 0) {
			if (quality->at(i) == 1) {
				INFO("Total complement");
			}
			else {
				INFO("Complement subpath " << quality->at(i));
				PrintPath(g, paths[i]);
				if (pairs->at(i) != -1) {
					PrintPath(g, paths[pairs->at(i)]);
				} else {
					INFO("No complment path");
				}
			}
			found.insert(i);
			found.insert(pairs->at(i));
		}
	}
}

//Remove overlaps, remove sub paths first
void RemoveOverlaps(Graph& g, std::vector<BidirectionalPath>& paths) {
	INFO("Removing overlaps");
    for (int k = 0; k < (int) paths.size(); k += 2) {
    		BidirectionalPath& path = paths[k];
            EdgeId lastEdge = path.back();

            int overlap = -1;
            int overlaped = - 1;
            for (int l = 0; l < (int) paths.size(); ++l) {
				if (k != l) {
					BidirectionalPath& toCompare = paths[l];

					for (int i = 0; i < (int) toCompare.size(); ++i) {
						if (lastEdge == toCompare[i]) {
							int diff = path.size() - i - 1;
							bool found = true;

							for (int j = i - 1; j >= 0; --j) {
								if (toCompare[j] != path[j + diff]) {
									found = false;
									break;
								}
							}

							if (found && overlap < i) {
								overlap = i;
								overlaped = l;
							}
					   }
					}
				}
            }

            if (overlap != -1) {
            	size_t overlapLength = 0;
				for (int i = 0; i <= overlap; ++i) {
					overlapLength += g.length(path[i]);
				}

            	INFO("Found overlap by " << overlap + 1 << " edge(s) with total length " << overlapLength);
            	PrintPath(g, path);
            	PrintPath(g, paths[overlaped]);

            	if (overlap >= (int) path.size() - 1) {
            		INFO("PATHS ABOVE ARE STRANGE!");
            	}

				overlap = std::min(overlap, (int) path.size() - 1);

				for (int i = 0; i <= overlap; ++i) {
					path.pop_back();
				}

				INFO("Same one removed from reverse-complement path");
				BidirectionalPath& comp = paths[path.conj_id];
				for (int i = 0; i <= overlap; ++i) {
					comp.pop_front();
				}
            }
    }
	INFO("Done");
}

void MakeBlackSet(Graph& g, Path<Graph::EdgeId>& path1, Path<Graph::EdgeId>& path2, std::set<EdgeId> blackSet) {
	for (auto edge1 = g.SmartEdgeBegin(); !edge1.IsEnd(); ++edge1) {
		if (!ContainsPath(path1, *edge1) && !ContainsPath(path2, *edge1)) {
			blackSet.insert(*edge1);
		}
	}
}

void MakeComplementPaths(Graph& g, BidirectionalPath& path1, BidirectionalPath& path2, int start, bool cutEnds) {
	if (cutEnds) {
		//If path2 starts 'before' path1
		for (int i = start; i < 0; ++i) {
			path2.pop_back();
		}
		//Else, two of these can never executed together
		for (int i = 0; i < start; ++i) {
			path1.pop_front();
		}

		int s1 = (int) path1.size();
		int s2 = (int) path2.size();

		//If path2 ends 'after' path1
		for (int i = s1; i < s2; ++i) {
			path2.pop_front();
		}
		//Else, two of these can never executed together
		for (int i = s2; i < s1; ++i) {
			path1.pop_back();
		}
	} else {
		//If path2 starts 'before' path1
		for (int i = start; i < 0; ++i) {
			path1.push_front(g.conjugate(path2[path2.size() + i]));
		}
		//Else, two of these can never executed together
		for (int i = 0; i < start; ++i) {
			path2.push_back(g.conjugate(path1[start - 1 - i]));
		}


		int s1 = (int) path1.size();
		int s2 = (int) path2.size();

		//If path2 ends 'after' path1
		for (int i = s1; i < s2; ++i) {
			path1.push_back(g.conjugate(path2[path2.size() - i - 1]));
		}
		//Else, two of these can never executed together
		for (int i = s2; i < s1; ++i) {
			path2.push_front(g.conjugate(path1[i]));
		}
	}

	INFO("Part are made complement");
	DetailedPrintPath(g, path1);
	DetailedPrintPath(g, path2);
}


void ResolveUnequalComplement(Graph& g, std::vector<BidirectionalPath>& paths, bool cutEnds, size_t minConjLen) {
	INFO("Making paths conjugate");
	for (int i = 0; i < (int) paths.size(); i += 2) {
		if (!ComplementPaths(g, paths[i], paths[i + 1])) {
			int start = 0;
			if (ContainsCommonComplementPath(g, paths[i], paths[i + 1], &start) != 0) {
				INFO("Found common complement path starting " << start);
				DetailedPrintPath(g, paths[i]);
				DetailedPrintPath(g, paths[i + 1]);
				MakeComplementPaths(g, paths[i], paths[i + 1], start, cutEnds);
			} else {
				INFO("Looking for common parts");
				DetailedPrintPath(g, paths[i]);
				DetailedPrintPath(g, paths[i + 1]);

				std::vector<BidirectionalPath> newPaths;
				FindAllComplementParts(g, paths[i], paths[i + 1], minConjLen, &newPaths);
				for (int j = 0; j < (int) newPaths.size(); j += 2) {
					AddPathPairToContainer(newPaths[j], newPaths[j+1], paths);
				}
			}
		}
	}
	INFO("Conjugate done");

}

bool PathAgreed(Graph& g, BidirectionalPath& path, PairedInfoIndices& pairedInfo, double threshold) {

	static size_t maxl = GetMaxInsertSize(pairedInfo) - K;
	static size_t minl = GetMinGapSize(pairedInfo) + K;

	DETAILED_INFO("Agreed stat, maxl = " << maxl << ", minl = " << minl);
	DetailedPrintPath(g, path);

	for (int i = 0; i < (int) path.size(); ++i) {
		DETAILED_INFO("Edge #" << i);

		BidirectionalPath edge;
		edge.push_back(path[i]);
		PathLengths length;
		length.push_back(g.length(path[i]));

		for (int j = i + 1; j < (int) path.size(); ++j) {
			if (length.back() - g.length(edge[0]) <= maxl &&
					length.back() + g.length(path[j]) >= minl) {

				double weight =
						ExtentionWeight(g, edge, length, path[j], pairedInfo, 0, true, false);

				DETAILED_INFO("With edge #" << j << ": " << weight);
				if (weight < threshold) {
					return false;
				}
			}

			length[0] += g.length(path[j]);
		}

		length.clear();
		length.push_back(0);

		for (int j = i - 1; j >= 0; --j) {
			if (length.back() <= maxl &&
					length.back() + g.length(edge[0]) + g.length(path[j]) >= minl) {

				double weight =
						ExtentionWeight(g, edge, length, path[j], pairedInfo, 0, false, false);

				DETAILED_INFO("With edge #" << j << ": " << weight);
				if (weight < threshold) {
					return false;
				}
			}

			length[0] += g.length(path[j]);
		}
	}

	return true;

}


void RemoveUnagreedPaths(Graph& g, std::vector<BidirectionalPath>& paths, PairedInfoIndices& pairedInfo, double threshold, std::vector<BidirectionalPath> * output) {
	output->clear();

	for (int i = 0; i < (int) paths.size(); i += 2) {
		if (PathAgreed(g, paths[i], pairedInfo, threshold) && PathAgreed(g, paths[i + 1], pairedInfo, threshold)) {
			AddPathPairToContainer(paths[i], paths[i+1], *output);
		}
	}

}


} // namespace long_contigs

#endif /* PATH_UTILS_HPP_ */
