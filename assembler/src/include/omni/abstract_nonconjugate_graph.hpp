#ifndef ABSTRACT_NONCONJUGATE_GRAPH_HPP_
#define ABSTRACT_NONCONJUGATE_GRAPH_HPP_

#include <vector>
#include <set>
#include <cstring>
#include "sequence/seq.hpp"
#include "sequence/sequence.hpp"
#include "logging.hpp"
#include "sequence/nucl.hpp"
//#include "strobe_read.hpp"
#include "io/paired_read.hpp"
#include "omni_utils.hpp"
#include "abstract_graph.hpp"

namespace omnigraph {

template<class DataMaster>
class AbstractNonconjugateGraph;

template<class DataMaster>
class SingleEdge;

template<class DataMaster>
class SingleVertex {
private:
	typedef SingleVertex<DataMaster>* VertexId;
	typedef SingleEdge<DataMaster>* EdgeId;
	typedef typename DataMaster::VertexData VertexData;

	friend class AbstractGraph<SingleVertex<DataMaster>*,
			SingleEdge<DataMaster>*, DataMaster, typename set<SingleVertex<
					DataMaster>*>::const_iterator> ;
	friend class AbstractNonconjugateGraph<DataMaster>;

	vector<EdgeId> outgoing_edges_;

	vector<EdgeId> incoming_edges_;

	VertexData data_;

	size_t OutgoingEdgeCount() const {
		return outgoing_edges_.size();
	}

	const vector<EdgeId> OutgoingEdges() const {
		return outgoing_edges_;
	}

	size_t IncomingEdgeCount() const {
		return incoming_edges_.size();
	}

	const vector<EdgeId> IncomingEdges() const {
		return incoming_edges_;
	}

	SingleVertex(VertexData data) :
		data_(data) {
	}

	VertexData &data() {
		return data_;
	}

	void set_data(VertexData data) {
		data_ = data;
	}

	//	bool IsDeadend() {
	//		return outgoing_edges_.size() == 0;
	//	}

	void AddOutgoingEdge(EdgeId e) {
		outgoing_edges_.push_back(e);
	}

	bool RemoveOutgoingEdge(const EdgeId e) {
		auto it = outgoing_edges_.begin();
		while (it != outgoing_edges_.end() && *it != e) {
			++it;
		}
		if (it == outgoing_edges_.end()) {
			return false;
		}
		outgoing_edges_.erase(it);
		return true;
	}

	//	bool IsDeadstart() {
	//		return incoming_edges_.size() == 0;
	//	}

	void AddIncomingEdge(EdgeId e) {
		incoming_edges_.push_back(e);
	}

	bool RemoveIncomingEdge(const EdgeId e) {
		auto it = incoming_edges_.begin();
		while (it != incoming_edges_.end() && *it != e) {
			++it;
		}
		if (it == incoming_edges_.end()) {
			return false;
		}
		incoming_edges_.erase(it);
		return true;
	}
	//todo remove if nobody uses
	const vector<EdgeId> OutgoingEdgesTo(VertexId v) const {
		vector<EdgeId> result;
		for (auto it = outgoing_edges_.begin(); it != outgoing_edges_.end(); ++it) {
			if ((*it)->end() == v) {
				result.push_back(*it);
			}
		}
		return result;
	}

	~SingleVertex() {
		VERIFY(outgoing_edges_.size() == 0);
	}
};

template<class DataMaster>
class SingleEdge {
private:
	typedef SingleVertex<DataMaster>* VertexId;
	typedef SingleEdge<DataMaster>* EdgeId;
	typedef typename DataMaster::EdgeData EdgeData;

	friend class AbstractGraph<SingleVertex<DataMaster>*,
			SingleEdge<DataMaster>*, DataMaster, typename set<SingleVertex<
					DataMaster>*>::const_iterator> ;
	friend class AbstractNonconjugateGraph<DataMaster>;
	//todo unfriend
	friend class SingleVertex<DataMaster> ;

	VertexId start_;
	VertexId end_;

	EdgeData data_;

	SingleEdge(VertexId start, VertexId end, const EdgeData &data) :
		start_(start), end_(end), data_(data) {
	}

	EdgeData &data() {
		return data_;
	}

	void set_data(EdgeData &data) {
		data_ = data;
	}

	VertexId start() const {
		return start_;
	}

	VertexId end() const {
		return end_;
	}

	~SingleEdge() {
	}
};

template<class DataMaster>
class AbstractNonconjugateGraph: public AbstractGraph<
		SingleVertex<DataMaster>*, SingleEdge<DataMaster>*, DataMaster,
		typename set<SingleVertex<DataMaster>*>::const_iterator> {
	typedef AbstractGraph<SingleVertex<DataMaster>*, SingleEdge<DataMaster>*,
			DataMaster, typename set<SingleVertex<DataMaster>*>::const_iterator>
			base;
public:
	typedef typename base::VertexId VertexId;
	typedef typename base::EdgeId EdgeId;
	typedef typename base::VertexData VertexData;
	typedef typename base::EdgeData EdgeData;
	typedef typename base::VertexIterator VertexIterator;

private:

	virtual VertexId HiddenAddVertex(const VertexData &data) {
		VertexId v = new SingleVertex<DataMaster> (data);
		this->vertices_.insert(v);
		return v;
	}

	virtual void HiddenDeleteVertex(VertexId v) {
		this->vertices_.erase(v);
		delete v;
	}

	virtual EdgeId HiddenAddEdge(VertexId v1, VertexId v2, const EdgeData &data) {
		VERIFY(this->vertices_.find(v1) != this->vertices_.end()
				&& this->vertices_.find(v2) != this->vertices_.end());
		EdgeId newEdge = new SingleEdge<DataMaster> (v1, v2, data);
		v1->AddOutgoingEdge(newEdge);
		v2->AddIncomingEdge(newEdge);
		return newEdge;
	}

	virtual void HiddenDeleteEdge(EdgeId edge) {
		VertexId start = edge->start();
		VertexId end = edge->end();
		start->RemoveOutgoingEdge(edge);
		end->RemoveIncomingEdge(edge);
		delete edge;
	}

	virtual vector<EdgeId> CorrectMergePath(const vector<EdgeId>& path) {
		return path;
	}

	virtual vector<EdgeId> EdgesToDelete(const vector<EdgeId> &path) {
		return path;
	}

	virtual vector<VertexId> VerticesToDelete(const vector<EdgeId> &path) {
		vector<VertexId> answer;
		for (size_t i = 0; i + 1 < path.size(); i++) {
			EdgeId e = path[i + 1];
			VertexId v = EdgeStart(e);
			answer.push_back(v);
		}
		return answer;
	}

public:

	bool SplitCondition(VertexId vertex, const vector<EdgeId> &splittingEdges) {
		return true;
	}

	AbstractNonconjugateGraph(const DataMaster& master) :
		base(new SimpleHandlerApplier<AbstractNonconjugateGraph> (), master) {
	}

	virtual ~AbstractNonconjugateGraph() {
		//		while (!this->vertices_.empty()) {
		//			ForceDeleteVertex(*this->vertices_.begin());
		//		}
		TRACE("~AbstractNonconjugateGraph")
		for (auto it = this->SmartVertexBegin();
				!it.IsEnd();
++			it) {
				ForceDeleteVertex(*it);
			}
			TRACE("~AbstractNonconjugateGraph ok")
		}

		/*virtual*/bool RelatedVertices(VertexId v1, VertexId v2) const {
			return v1 == v2;
		}

		pair<VertexId, vector<pair<EdgeId, EdgeId>>> SplitVertex(VertexId vertex, vector<EdgeId> splittingEdges) {
			vector<double> split_coefficients(splittingEdges.size(),1);
			return SplitVertex(vertex, splittingEdges, split_coefficients);
		}

		pair<VertexId, vector<pair<EdgeId, EdgeId>>> SplitVertex(VertexId vertex, vector<EdgeId> &splittingEdges, vector<double> &split_coefficients) {
			//TODO:: check whether we handle loops correctly!
			VertexId newVertex = HiddenAddVertex(vertex->data());
			vector<pair<EdgeId, EdgeId>> edge_clones;
			for (size_t i = 0; i < splittingEdges.size(); i++) {
				VertexId start_v = this->EdgeStart(splittingEdges[i]);
				VertexId start_e = this->EdgeEnd(splittingEdges[i]);
				if (start_v == vertex)
				start_v = newVertex;
				if (start_e == vertex)
				start_e = newVertex;
				EdgeId newEdge = HiddenAddEdge(start_v, start_e, splittingEdges[i]->data());
				edge_clones.push_back(make_pair(splittingEdges[i], newEdge));
			}
			//FIRE
			FireVertexSplit(newVertex, edge_clones, split_coefficients, vertex);
			FireAddVertex(newVertex);
			for(size_t i = 0; i < splittingEdges.size(); i ++)
			FireAddEdge(edge_clones[i].second);
			return make_pair(newVertex, edge_clones);
		}

	private:
		DECL_LOGGER("AbstractNonconjugateGraph")
	};

}
#endif /* ABSTRACT_NONCONJUGATE_GRAPH_HPP_ */
