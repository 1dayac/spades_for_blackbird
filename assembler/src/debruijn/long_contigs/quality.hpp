/*
 * quality.hpp
 *
 *  Created on: Aug 3, 2011
 *      Author: andrey
 */

#ifndef QUALITY_HPP_
#define QUALITY_HPP_

#include "lc_common.hpp"

namespace long_contigs {

using namespace debruijn_graph;


// ====== Quality functions ======
//Find bidirectional path in given genome path
int FindInGenomePath(BidirectionalPath& myPath, Path<Graph::EdgeId>& genomePath) {
	if (myPath.size() > genomePath.size()) {
		INFO("Warning, unexpected path length")
		return -1;
	}

	for (size_t i = 0; i < genomePath.size() - myPath.size() + 1 ; ++i) {
		bool found = true;

		for (size_t j = 0; j < myPath.size(); ++j) {
			if (myPath[j] != genomePath[i + j]) {
				found = false;
				break;
			}
		}

		if (found) {
			return (int) i;
		}
	}
	return -1;
}

//Find inexact match to genome path
size_t FindInGenomeInexact(Graph& g, BidirectionalPath& myPath, Path<Graph::EdgeId>& genomePath, int& startPos, size_t& maxLengthMached) {
	if (myPath.size() > genomePath.size()) {
		INFO("Warning, unexpected path length")
		return -1;
	}

	size_t maxEdgesMatched = 0;
	for (size_t i = 0; i < genomePath.size(); ++i) {
		size_t edgesMatched = 0;
		size_t lengthMatched = 0;

		for (size_t j = 0; j < myPath.size() && i + j < genomePath.size(); ++j) {
			if (myPath[j] == genomePath[i + j]) {
				++edgesMatched;
				lengthMatched += g.length(myPath[j]);
			}
		}

		if (edgesMatched > maxEdgesMatched) {
			maxEdgesMatched = edgesMatched;
			maxLengthMached = lengthMatched;
			startPos = i;
		}
	}

	return maxEdgesMatched;
}


//Count all paths in genome paths
template<size_t k>
size_t PathsInGenome(Graph& g, const EdgeIndex<k + 1, Graph>& index, const Sequence& genome, std::vector<BidirectionalPath>& paths,
		Path<typename Graph::EdgeId>& path1, Path<typename Graph::EdgeId>& path2, bool displayInexactPaths = false) {

	size_t pathCount = 0;
	for(auto iter = paths.begin(); iter != paths.end(); ++iter) {

		int s = FindInGenomePath(*iter, path1);
		if (s != -1) {
			++pathCount;
			INFO("Path of length " << PathLength(g, *iter)  << " with " << iter->size() << " edges is found in genome path starting from edge " << s)
		}

		else {
			s = FindInGenomePath(*iter, path2);
			if (s != -1) {
				++pathCount;
				INFO("Path of length " << PathLength(g, *iter) << " with " << iter->size() << " edges is found in !genome path starting from edge " << s)
			}

			else {
				int pos1 = 0, pos2 = 0;
				size_t len1 = 0, len2 = 0;
				size_t edges1 = FindInGenomeInexact(g, *iter, path1, pos1, len1);
				size_t edges2 = FindInGenomeInexact(g, *iter, path2, pos2, len2);

				if (edges1 > edges2) {
					INFO("Path partly found, edges matched " << edges1 << "/" << iter->size() <<
							", length matched " << len1 << "/" << PathLength(g, *iter));

					if (displayInexactPaths) {
						PrintPath(g, *iter);
						PrintPathFromTo(g, path1, pos1, pos1 + iter->size());
					}
				}
				else {
					INFO("Path partly found, edges matched " << edges2 << "/" << iter->size() <<
												", length matched " << len2 << "/" << PathLength(g, *iter));

					if (displayInexactPaths) {
						PrintPath(g, *iter);
						PrintPathFromTo(g, path2, pos2, pos2 + iter->size());
					}
				}

			}
		}
	}
	return pathCount;
}

//Count all paths in genome paths
template<size_t k>
size_t PathsInGenome(Graph& g, const EdgeIndex<k + 1, Graph>& index, const Sequence& genome, std::vector<BidirectionalPath>& paths, bool displayInexactPaths = false) {
	Path<typename Graph::EdgeId> path1 = FindGenomePath<k> (genome, g, index);
	Path<typename Graph::EdgeId> path2 = FindGenomePath<k> (!genome, g, index);

	return PathsInGenome(g, index, genome, paths, path1, path2, displayInexactPaths);
}


} // namespace long_contigs


#endif /* QUALITY_HPP_ */
