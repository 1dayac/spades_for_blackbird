//***************************************************************************
//* Copyright (c) 2011-2012 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

/*
 * bulge_remover.hpp
 *
 *  Created on: Apr 13, 2011
 *      Author: sergey
 */

#pragma once

#include <cmath>
#include <stack>
#include "standard_base.hpp"
#include "omni_utils.hpp"
#include "graph_component.hpp"
#include "xmath.h"
#include "sequence/sequence_tools.hpp"
#include "path_processor.hpp"
#include "sequential_algorithm.hpp"

namespace omnigraph {

template<class Graph>
struct SimplePathCondition {
	typedef typename Graph::EdgeId EdgeId;
	const Graph& g_;

	SimplePathCondition(const Graph& g) :
			g_(g) {

	}

	bool operator()(EdgeId edge, const vector<EdgeId>& path) const {
		if (edge == g_.conjugate(edge))
			return false;
		for (size_t i = 0; i < path.size(); ++i)
			if (edge == path[i] || edge == g_.conjugate(path[i]))
				return false;
		for (size_t i = 0; i < path.size(); ++i) {
			if (path[i] == g_.conjugate(path[i])) {
				return false;
			}
			for (size_t j = i + 1; j < path.size(); ++j)
				if (path[i] == path[j] || path[i] == g_.conjugate(path[j]))
					return false;
		}
		return true;
	}
};

template<class Graph>
bool TrivialCondition(typename Graph::EdgeId edge,
		const vector<typename Graph::EdgeId>& path) {
	typedef typename Graph::EdgeId EdgeId;
	for (size_t i = 0; i < path.size(); ++i)
		for (size_t j = i + 1; j < path.size(); ++j)
			if (path[i] == path[j])
				return false;
	return true;
}

template<class Graph>
class MostCoveredAlternativePathChooser: public PathProcessor<Graph>::Callback {
	typedef typename Graph::EdgeId EdgeId;
	typedef typename Graph::VertexId VertexId;

	Graph& g_;
	EdgeId forbidden_edge_;
	double max_coverage_;
	vector<EdgeId> most_covered_path_;

	double PathAvgCoverage(const vector<EdgeId>& path) {
		double unnormalized_coverage = 0;
		size_t path_length = 0;
		for (size_t i = 0; i < path.size(); ++i) {
			EdgeId edge = path[i];
			size_t length = g_.length(edge);
			path_length += length;
			unnormalized_coverage += g_.coverage(edge) * length;
		}
		return unnormalized_coverage / path_length;
	}

public:

	MostCoveredAlternativePathChooser(Graph& g, EdgeId edge) :
			g_(g), forbidden_edge_(edge), max_coverage_(-1.0) {

	}

	virtual void HandlePath(const vector<EdgeId>& path) {
		double path_cov = PathAvgCoverage(path);
		for (size_t i = 0; i < path.size(); i++) {
			if (path[i] == forbidden_edge_)
				return;
		}
		if (path_cov > max_coverage_) {
			max_coverage_ = path_cov;
			most_covered_path_ = path;
		}
	}

	double max_coverage() {
		return max_coverage_;
	}

	const vector<EdgeId>& most_covered_path() {
		return most_covered_path_;
	}
};

/**
 * This class removes simple bulges from given graph with the following algorithm: it iterates through all edges of
 * the graph and for each edge checks if this edge is likely to be a simple bulge
 * if edge is judged to be one it is removed.
 */
template<class Graph>
class BulgeRemover : public SequentialAlgorithm<typename Graph::EdgeId> {

	typedef typename Graph::EdgeId EdgeId;
	typedef typename Graph::VertexId VertexId;


public:

	typedef boost::function<bool(EdgeId edge, const vector<EdgeId>& path)> BulgeCallbackF;


	BulgeRemover(
			Graph& graph,
			size_t max_length,
			double max_coverage,
			double max_relative_coverage,
			double max_delta,
			double max_relative_delta,
			BulgeCallbackF bulge_condition,
			BulgeCallbackF opt_callback = 0,
			boost::function<void(EdgeId)> removal_handler = 0)
				: graph_(graph),
				  max_length_(max_length),
				  max_coverage_(max_coverage),
				  max_relative_coverage_(max_relative_coverage),
				  max_delta_(max_delta),
				  max_relative_delta_(max_relative_delta),
				  bulge_condition_(bulge_condition),
				  opt_callback_(opt_callback),
				  removal_handler_(removal_handler) {
	}

	~BulgeRemover() {
	}

	bool PossibleBulgeEdge(EdgeId e);

	size_t PathLength(const vector<EdgeId>& path);

	virtual bool ProcessNext(const EdgeId& edge);

	virtual void Preprocessing() {
		TRACE("Bulge remove process started");
	}

	void RemoveBulges() {
		this->Preprocessing();

		CoverageComparator<Graph> comparator(graph_);
		for (auto iterator = graph_.SmartEdgeBegin(comparator); !iterator.IsEnd();
				++iterator) {

			this->ProcessNext(*iterator);
		}

		this->Postprocessing();
	}


	/**
	 * Returns most covered path from start to the end such that its length doesn't exceed length_left.
	 * Returns pair of empty vector and -1 if no such path could be found.
	 * Edges are returned in reverse order!
	 */
	//	pair<vector<EdgeId> , int> BestPath(VertexId start,
	//			VertexId end, int length_left);
	/**
	 * Checks if alternative path is simple (doesn't contain conjugate edges, edge e or conjugate(e))
	 * and its average coverage is greater than max_relative_coverage_ * g.coverage(e)
	 */
	bool BulgeCondition(EdgeId e, const vector<EdgeId>& path, double path_coverage) {
		return
			math::ge(path_coverage * max_relative_coverage_, graph_.coverage(e))
			&& bulge_condition_(e, path);
	}

	virtual bool TryToProcessBulge(EdgeId edge, const vector<EdgeId>& path) {
//		if (!graph_.IsInternalSafe(edge) || !graph_.IsInternalSafe(path)) {
//			// for algorithm to process this edge sequently.
//			graph_.InvalidateComponent();
//		}
//
//		if (graph_.IsValid()) {
		if (!graph_.IsInComponentSafe(edge) || !graph_.IsInComponentSafe(path)) {
//			if (!graph_.IsInternalSafe(edge) || !graph_.IsInternalSafe(path)) {
			return false;
		}

			if (opt_callback_)
				opt_callback_(edge, path);

			if (removal_handler_)
				removal_handler_(edge);

			VertexId start = graph_.EdgeStart(edge);
			VertexId end = graph_.EdgeEnd(edge);

			TRACE("Projecting edge " << graph_.str(edge));
			ProcessBulge(edge, path);

			TRACE("Compressing start vertex " << graph_.str(start));
			graph_.CompressVertex(start);

			TRACE("Compressing end vertex " << graph_.str(end));
			graph_.CompressVertex(end);

			return true;
//		} else {
//			TRACE("Component is invalid. " << "Edge "  << edge << " can not be processed in parallel.");
//			return false;
//		}
	}

	void ProcessBulge(EdgeId edge, const vector<EdgeId>& path) {

		EnsureEndsPositionAligner aligner(PathLength(path), graph_.length(edge));
		double prefix_length = 0.;
		vector<size_t> bulge_prefix_lengths;

		for (auto it = path.begin(); it != path.end(); ++it) {
			prefix_length += graph_.length(*it);
			bulge_prefix_lengths.push_back(aligner.GetPosition(prefix_length));
		}

		EdgeId edge_to_split = edge;
		size_t prev_length = 0;

		TRACE("Process bulge " << path.size() << " edges");

		for (size_t i = 0; i < path.size(); ++i) {
			if (bulge_prefix_lengths[i] > prev_length) {
				if (bulge_prefix_lengths[i] - prev_length != graph_.length(edge_to_split)) {

					TRACE("SplitEdge " << graph_.str(edge_to_split));
					TRACE("Start: " << graph_.str(graph_.EdgeStart(edge_to_split)));
					TRACE("Start: " << graph_.str(graph_.EdgeEnd(edge_to_split)));

					pair<EdgeId, EdgeId> split_result =
						graph_.SplitEdge(edge_to_split,	bulge_prefix_lengths[i] - prev_length);

					edge_to_split = split_result.second;

					TRACE("GlueEdges " << graph_.str(split_result.first));
					graph_.GlueEdges(split_result.first, path[i]);

				} else {
					TRACE("GlueEdges " << graph_.str(edge_to_split));
					graph_.GlueEdges(edge_to_split, path[i]);
				}
			}
			prev_length = bulge_prefix_lengths[i];
		}
	}


private:
	Graph& graph_;
	size_t max_length_;
	double max_coverage_;
	double max_relative_coverage_;
	double max_delta_;
	double max_relative_delta_;
	BulgeCallbackF bulge_condition_;
	BulgeCallbackF opt_callback_;
	boost::function<void(EdgeId)> removal_handler_;


private:
	DECL_LOGGER("BulgeRemover")
};

template<class Graph>
bool BulgeRemover<Graph>::PossibleBulgeEdge(EdgeId e) {
	return graph_.length(e) <= max_length_ && graph_.coverage(e) < max_coverage_;
}

template<class Graph>
size_t BulgeRemover<Graph>::PathLength(const vector<EdgeId>& path) {
	size_t length = 0;
	for (size_t i = 0; i < path.size(); ++i) {
		length += graph_.length(path[i]);
	}
	return length;
}

template<class Graph>
bool BulgeRemover<Graph>::ProcessNext(const EdgeId& edge) {

//	CoverageComparator<Graph> comparator(graph_);

	TRACE(
		"Considering edge " << graph_.str(edge) <<
		" of length " << graph_.length(edge) <<
		" and avg coverage " << graph_.coverage(edge)
	);

	TRACE("Is possible bulge " << PossibleBulgeEdge(edge));

	if (!PossibleBulgeEdge(edge)) {
		TRACE("-----------------------------------");
		return true;
	}

	size_t kplus_one_mer_coverage = math::round(graph_.length(edge) * graph_.coverage(edge));
	TRACE("Processing edge " << graph_.str(edge) << " and coverage " << kplus_one_mer_coverage);

	VertexId start = graph_.EdgeStart(edge);
	TRACE("Start " << graph_.str(start));

	VertexId end = graph_.EdgeEnd(edge);
	TRACE("End " << graph_.str(end));

	size_t delta = std::floor(
		std::max(
			max_relative_delta_ * graph_.length(edge),
			max_delta_
		)
	);

	MostCoveredAlternativePathChooser<Graph> path_chooser(graph_, edge);

	PathProcessor<Graph> path_finder(
		graph_,
		(graph_.length(edge) > delta) ? graph_.length(edge) - delta : 0,
		graph_.length(edge) + delta,
		start,
		end,
		path_chooser
	);

	path_finder.Process();

	const vector<EdgeId>& path = path_chooser.most_covered_path();
	double path_coverage = path_chooser.max_coverage();

	TRACE("Best path with coverage " << path_coverage << " is " << PrintPath<Graph>(graph_, path));

	//if edge was returned, this condition will fail
	if (BulgeCondition(edge, path, path_coverage)) {
		TRACE("Satisfied condition");

		return TryToProcessBulge(edge, path);
	} else {
		TRACE("Didn't satisfy condition");
	}

	TRACE("-----------------------------------");
	return true;
}
}
