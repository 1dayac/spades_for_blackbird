/*
 * visualize.hpp
 *
 *  Created on: Aug 4, 2011
 *      Author: andrey
 */

#ifndef VISUALIZE_HPP_
#define VISUALIZE_HPP_

#include "lc_common.hpp"

namespace long_contigs {

using namespace debruijn_graph;

template<class Graph>
class PathsGraphLabeler : public AbstractGraphLabeler<Graph> {
	typedef AbstractGraphLabeler<Graph> base;
	typedef typename Graph::EdgeId EdgeId;
	typedef typename Graph::VertexId VertexId;

	const std::vector<BidirectionalPath>& paths_;
	std::map<EdgeId, std::string> labels_;

public:
	PathsGraphLabeler(const Graph& g, const std::vector<BidirectionalPath>& paths) : base(g), paths_(paths){
//		for (auto iter = g.SmartEdgeBegin(); !iter.IsEnd(); ++iter) {
//			labels_[*iter] = "";
//		}

		for(size_t i = 0; i < paths.size(); ++i) {
			const BidirectionalPath& path = paths[i];

			for (size_t idx = 0; idx < path.size(); ++idx) {
				if (labels_[path[idx]].size() > 0) {
					labels_[path[idx]] += ", ";
				}
				labels_[path[idx]] += "(" + ToString(paths[i].uid) + " : " + ToString(idx) + ")";
			}
		}
	}

	virtual std::string label(VertexId vertexId) const {
		return "";
	}

	virtual std::string label(EdgeId edgeId) const {
		auto label = labels_.find(edgeId);
		return label == labels_.end() ? "" : label->second;
	}
};

void WritePathLocality(const conj_graph_pack& gp, const GraphLabeler<Graph>& labeler,
		const string& folder, const BidirectionalPath& path, size_t edge_split_length
		, const Path<EdgeId>& color1, const Path<EdgeId>& color2) {
	WriteComponentsAlongPath(gp.g, labeler, folder + ToString(path.uid) + ".dot", "graph" + ToString(path.uid)
			, edge_split_length
			, as_trivial_mapping_path(gp.g, as_simple_path(gp.g, path))
			, color1, color2);
}

void WritePathLocalities(const conj_graph_pack& gp, const string& folder, const std::vector<BidirectionalPath>& paths) {
	auto path1 = FindGenomeMappingPath<K> (gp.genome, gp.g, gp.index, gp.kmer_mapper);
	auto path2 = FindGenomeMappingPath<K> (!gp.genome, gp.g, gp.index, gp.kmer_mapper);

	for (auto it = paths.begin(); it != paths.end(); ++it) {
		if (it->size() > 1) {
			/////todo code duplication
			StrGraphLabeler<Graph> str_labeler(gp.g);
			PathsGraphLabeler<Graph> path_labeler(gp.g, vector<BidirectionalPath>{*it});
			EdgePosGraphLabeler<Graph> pos_labeler(gp.g, gp.edge_pos);

			CompositeLabeler<Graph> composite_labeler(str_labeler, path_labeler, pos_labeler);
			/////todo code duplication

			string path_folder = folder + ToString(it->uid) + "/";
			make_dir(path_folder);
			WritePathLocality(gp, composite_labeler, path_folder, *it, 1000, path1.simple_path(), path2.simple_path());
		}
	}
}

void WriteGraphWithPathsSimple(const conj_graph_pack& gp, const string& file_name, const string& graph_name, const std::vector<BidirectionalPath>& paths) {

	Path<Graph::EdgeId> path1 = FindGenomePath<K> (gp.genome, gp.g, gp.index);
	Path<Graph::EdgeId> path2 = FindGenomePath<K> (!gp.genome, gp.g, gp.index);

	std::fstream filestr;
	filestr.open(file_name.c_str(), std::fstream::out);

	gvis::DotGraphPrinter<Graph::VertexId> printer(graph_name, filestr);
	PathColorer<Graph> path_colorer(gp.g, path1, path2);

	map<Graph::EdgeId, string> coloring = path_colorer.ColorPath();

	StrGraphLabeler<Graph> str_labeler(gp.g);
	PathsGraphLabeler<Graph> path_labeler(gp.g, paths);
	EdgePosGraphLabeler<Graph> pos_labeler(gp.g, gp.edge_pos);

	CompositeLabeler<Graph> composite_labeler(str_labeler, path_labeler, pos_labeler);

	BorderVertexColorer<Graph> v_colorer(gp.g);
	ColoredGraphVisualizer<Graph> gv(gp.g, printer, composite_labeler, coloring, v_colorer);
	AdapterGraphVisualizer<Graph> result_vis(gp.g, gv);
	result_vis.Visualize();
	filestr.close();
}

} // namespace long_contigs

#endif /* VISUALIZE_HPP_ */
