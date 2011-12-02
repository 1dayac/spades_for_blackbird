#pragma once

#include "io/paired_read.hpp"
#include "seq_map.hpp"
#include "omni/omni_utils.hpp"
#include "omni/id_track_handler.hpp"
#include "logging.hpp"
#include "omni/paired_info.hpp"
#include "xmath.h"
#include <boost/optional.hpp>
#include <iostream>
#include "sequence/sequence_tools.hpp"
#include "omni/splitters.hpp"

#include "new_debruijn.hpp"
//#include "common/io/paired_read.hpp"
namespace debruijn_graph {

using omnigraph::Path;
using omnigraph::MappingPath;
using omnigraph::Range;
using omnigraph::MappingRange;
using omnigraph::PairInfo;
using omnigraph::GraphActionHandler;
//using io::PairedRead;

template<size_t kmer_size_, typename Graph>
class ReadThreaderResult {
	typedef typename Graph::EdgeId EdgeId;

	Path<EdgeId> left_read_, right_read_;
	int gap_;
public:
	ReadThreaderResult(Path<EdgeId> left_read, Path<EdgeId> right_read, int gap) :
			gap_(gap), left_read_(left_read), right_read_(right_read) {
	}
};

template<typename Graph>
class SingleReadThreaderResult {
	typedef typename Graph::EdgeId EdgeId;
public:
	EdgeId edge_;
	int read_position_;
	int edge_position_;
	SingleReadThreaderResult(EdgeId edge, int read_position, int edge_position) :
			edge_(edge), read_position_(read_position), edge_position_(
					edge_position) {
	}
};

template<typename Graph>
class ReadMappingResult {
public:
	Sequence read_;
	vector<SingleReadThreaderResult<Graph> > res_;
	ReadMappingResult(Sequence read,
			vector<SingleReadThreaderResult<Graph> > res) :
			read_(read), res_(res) {

	}
	ReadMappingResult() {

	}
};
/**
 * DataHashRenewer listens to add/delete events and updates index according to those events. This class
 * can be used both with vertices and edges of graph.
 * todo EdgeNucls are hardcoded!
 */
template<size_t kmer_size_, typename Graph, typename ElementId>
class DataHashRenewer {

	typedef Seq<kmer_size_> Kmer;
	typedef SeqMap<kmer_size_, ElementId> Index;
	const Graph &g_;

	Index &index_;

	/**
	 *	renews hash for vertex and complementary
	 *	todo renew not all hashes
	 */
	void RenewKmersHash(ElementId id) {
		Sequence nucls = g_.EdgeNucls(id);
		//		DEBUG("Renewing hashes for k-mers of sequence " << nucls);
		index_.RenewKmersHash(nucls, id);
	}

	void DeleteKmersHash(ElementId id) {
		Sequence nucls = g_.EdgeNucls(id);
		//		DEBUG("Deleting hashes for k-mers of sequence " << nucls);
		index_.DeleteKmersHash(nucls, id);
	}

public:
	/**
	 * Creates DataHashRenewer for specified graph and index
	 * @param g graph to be indexed
	 * @param index index to be synchronized with graph
	 */
	DataHashRenewer(const Graph& g, Index& index) :
			g_(g), index_(index) {
	}

	virtual ~DataHashRenewer() {

	}

	void HandleAdd(ElementId id) {
		RenewKmersHash(id);
	}

	virtual void HandleDelete(ElementId id) {
		DeleteKmersHash(id);
	}

private:
	DECL_LOGGER("DataHashRenewer")
};

/**
 * EdgeIndex is a structure to store info about location of certain k-mers in graph. It delegates all
 * container procedures to inner_index_ which is SeqMap and all handling procedures to
 * renewer_ which is DataHashRenewer.
 * @see SeqMap
 * @see DataHashRenewer
 */
template<size_t k, class Graph>
class EdgeIndex: public GraphActionHandler<Graph> {
	typedef typename Graph::EdgeId EdgeId;
	typedef SeqMap<k, EdgeId> InnerIndex;
	typedef Seq<k> Kmer;
	InnerIndex inner_index_;
	DataHashRenewer<k, Graph, EdgeId> renewer_;
	bool delete_index_;
public:

	EdgeIndex(const Graph& g) :
			GraphActionHandler<Graph>(g, "EdgeIndex"), inner_index_(), renewer_(
					g, inner_index_), delete_index_(true) {
	}

	virtual ~EdgeIndex() {
		TRACE("~EdgeIndex OK")
	}

	InnerIndex &inner_index() {
		return inner_index_;
	}

	virtual void HandleAdd(EdgeId e) {
		renewer_.HandleAdd(e);
	}

	virtual void HandleDelete(EdgeId e) {
		renewer_.HandleDelete(e);
	}

	virtual void HandleGlue(EdgeId new_edge, EdgeId edge1, EdgeId edge2) {
	}

	bool containsInIndex(const Kmer& kmer) const {
		return inner_index_.containsInIndex(kmer);
	}

	const pair<EdgeId, size_t>& get(const Kmer& kmer) const {
		return inner_index_.get(kmer);
	}

};

template<size_t k, class Graph>
class KmerMapper: public omnigraph::GraphActionHandler<Graph> {
	typedef omnigraph::GraphActionHandler<Graph> base;
	typedef typename Graph::EdgeId EdgeId;
	typedef Seq<k> Kmer;
	typedef typename std::tr1::unordered_map<Kmer, Kmer, typename Kmer::hash> MapType;

	void RemapKmers(const Sequence& old_s, const Sequence& new_s) {
		//		cout << endl << "Mapping " << old_s << " to " << new_s << endl;
		UniformPositionAligner aligner(old_s.size() - k + 1,
				new_s.size() - k + 1);
		Kmer old_kmer = old_s.start<k>() >> 0;
		for (size_t i = k - 1; i < old_s.size(); ++i) {
			old_kmer = old_kmer << old_s[i];
			size_t old_kmer_offset = i - k + 1;
			size_t new_kmer_offest = aligner.GetPosition(old_kmer_offset);
			Kmer new_kmer(new_s, new_kmer_offest);
			mapping_[old_kmer] = new_kmer;
			//			cout << "Kmer " << old_kmer << " mapped to " << new_kmer << endl;
		}
	}

	MapType mapping_;

public:
	KmerMapper(const Graph& g) :
			base(g, "KmerMapper") {

	}

	virtual ~KmerMapper() {

	}

	virtual void HandleGlue(EdgeId new_edge, EdgeId edge1, EdgeId edge2) {
		VERIFY(this->g().EdgeNucls(new_edge) == this->g().EdgeNucls(edge2));
		RemapKmers(this->g().EdgeNucls(edge1), this->g().EdgeNucls(edge2));
	}

	Kmer Substitute(const Kmer& kmer) const {
		Kmer answer = kmer;
		auto it = mapping_.find(answer);
		while (it != mapping_.end()) {
			VERIFY(it->first != it->second);
			answer = (*it).second;
			it = mapping_.find(answer);
		}
		return answer;
	}

	void BinWrite(std::ostream& file) const {
		u_int32_t size = mapping_.size();
		file.write((const char *) &size, sizeof(u_int32_t));
		for (auto iter = mapping_.begin(); iter != mapping_.end(); ++iter) {
			Kmer::BinWrite(file, iter->first);
			Kmer::BinWrite(file, iter->second);
		}
	}

	void BinRead(std::istream& file) {
		mapping_.clear();
		u_int32_t size;

		file.read((char *) &size, sizeof(u_int32_t));
		for (u_int32_t i = 0; i < size; ++i) {
			Kmer key;
			Kmer value;
			Kmer::BinRead(file, &key);
			Kmer::BinRead(file, &value);
			mapping_[key] = value;
		}
	}

	bool CompareTo(KmerMapper<k, Graph> const& m) {
		if (mapping_.size() != m.mapping_.size()) {
			INFO("Unequal sizes");
		}
		for (auto iter = mapping_.begin(); iter != mapping_.end(); ++iter) {
			auto cmp = m.mapping_.find(iter->first);
			if (cmp == m.mapping_.end() || cmp->second != iter->second) {
				return false;
			}
		}
		return true;
	}

	void clear() {
		mapping_.clear();
	}
};

/**
 * This class finds how certain sequence is mapped to genome. As it is now it works correct only if sequence
 * is mapped to graph ideally and in unique way.
 */
template<size_t k, class Graph>
class SimpleSequenceMapper {
public:
	typedef typename Graph::EdgeId EdgeId;
	typedef Seq<k> Kmer;
	typedef EdgeIndex<k, Graph> Index;
private:
	const Graph& g_;
	const Index &index_;

	bool TryThread(Kmer &kmer, vector<EdgeId> &passed,
			size_t &endPosition) const {
		EdgeId last = passed[passed.size() - 1];
		if (endPosition + 1 < g_.length(last)) {
			if (g_.EdgeNucls(last)[endPosition + k] == kmer[k - 1]) {
				endPosition++;
				return true;
			}
		} else {
			vector<EdgeId> edges = g_.OutgoingEdges(g_.EdgeEnd(last));
			for (size_t i = 0; i < edges.size(); i++) {
				if (g_.EdgeNucls(edges[i])[k - 1] == kmer[k - 1]) {
					passed.push_back(edges[i]);
					endPosition = 0;
					return true;
				}
			}
		}
		return false;
	}

	bool FindKmer(Kmer kmer, vector<EdgeId> &passed, size_t &startPosition,
			size_t &endPosition) const {
		if (index_.containsInIndex(kmer)) {
			pair<EdgeId, size_t> position = index_.get(kmer);
			endPosition = position.second;
			if (passed.empty()) {
				startPosition = position.second;
			}
			if (passed.empty() || passed.back() != position.first) {
				passed.push_back(position.first);
			}
			return true;
		}
		return false;
	}

	bool ProcessKmer(Kmer &kmer, vector<EdgeId> &passed, size_t &startPosition,
			size_t &endPosition, bool valid) const {
		if (valid) {
			return TryThread(kmer, passed, endPosition);
		} else {
			return FindKmer(kmer, passed, startPosition, endPosition);
		}
	}

public:
	/**
	 * Creates SimpleSequenceMapper for given graph. Also requires index_ which should be synchronized
	 * with graph.
	 * @param g graph sequences should be mapped to
	 * @param index index synchronized with graph
	 */
	SimpleSequenceMapper(const Graph& g, const Index& index) :
			g_(g), index_(index) {
	}

	/**
	 * Finds a path in graph which corresponds to given sequence.
	 * @read sequence to be mapped
	 */

	Path<EdgeId> MapSequence(const Sequence &read) const {
		vector<EdgeId> passed;
		if (read.size() <= k - 1) {
			return Path<EdgeId>();
		}
		Kmer kmer = read.start<k>();
		size_t startPosition = -1;
		size_t endPosition = -1;
		bool valid = ProcessKmer(kmer, passed, startPosition, endPosition,
				false);
		for (size_t i = k; i < read.size(); ++i) {
			kmer = kmer << read[i];
			valid = ProcessKmer(kmer, passed, startPosition, endPosition,
					valid);
		}
		return Path<EdgeId>(passed, startPosition, endPosition + 1);
	}

};

template<size_t k, class Graph>
class ExtendedSequenceMapper {
public:
	typedef typename Graph::EdgeId EdgeId;
	typedef vector<MappingRange> RangeMappings;
	typedef Seq<k> Kmer;
	typedef EdgeIndex<k, Graph> Index;
	typedef KmerMapper<k, Graph> KmerSubs;

private:
	const Graph& g_;
	const IdTrackHandler<Graph>& int_ids_;
	const Index& index_;
	const KmerSubs& kmer_mapper_;

	void FindKmer(Kmer kmer, size_t kmer_pos, vector<EdgeId> &passed,
			RangeMappings& range_mappings) const {

		if (index_.containsInIndex(kmer)) {
			pair<EdgeId, size_t> position = index_.get(kmer);
			if (passed.empty() || passed.back() != position.first
					|| kmer_pos != range_mappings.back().initial_range.end_pos
					|| position.second + 1
							< range_mappings.back().mapped_range.end_pos) {
				passed.push_back(position.first);
				MappingRange mapping_range(Range(kmer_pos, kmer_pos + 1),
						Range(position.second, position.second + 1));
				range_mappings.push_back(mapping_range);
			} else {
				range_mappings.back().initial_range.end_pos = kmer_pos + 1;
				range_mappings.back().mapped_range.end_pos = position.second
						+ 1;
			}
		}
	}

	void ProcessKmer(Kmer kmer, size_t kmer_pos, vector<EdgeId> &passed,
			RangeMappings& interval_mapping) const {
		kmer = kmer_mapper_.Substitute(kmer);
		FindKmer(kmer, kmer_pos, passed, interval_mapping);
	}

public:
	ExtendedSequenceMapper(const Graph& g, /*todo delete*/
			const IdTrackHandler<Graph>& int_ids, const Index& index,
			const KmerSubs& kmer_mapper) :
			g_(g), int_ids_(int_ids), index_(index), kmer_mapper_(kmer_mapper) {
	}

	MappingPath<EdgeId> MapSequence(const Sequence &sequence) const {
		vector<EdgeId> passed_edges;
		RangeMappings range_mapping;

		if (sequence.size() < k) {
			return MappingPath<EdgeId>();
		}
		Kmer kmer = sequence.start<k>() >> 0;
		for (size_t i = k - 1; i < sequence.size(); ++i) {
			kmer = kmer << sequence[i];
			ProcessKmer(kmer, i - k + 1, passed_edges, range_mapping);
		}

		//DEBUG
//		for (size_t i = 0; i < passed_edges.size(); ++i) {
//			cerr << int_ids_.ReturnIntId(passed_edges[i]) << " (" << range_mapping[i] << ")"<< "; ";
//		}
//		cerr << endl;
		//DEBUG

		return MappingPath<EdgeId>(passed_edges, range_mapping);
	}
};

//todo compare performance
template<size_t k, class Graph>
class NewExtendedSequenceMapper {
public:
	typedef typename Graph::EdgeId EdgeId;
	typedef vector<MappingRange> RangeMappings;
	typedef Seq<k> Kmer;
	typedef EdgeIndex<k, Graph> Index;
	typedef KmerMapper<k, Graph> KmerSubs;

private:
	const Graph& g_;
	const IdTrackHandler<Graph>& int_ids_;
	const Index& index_;
	const KmerSubs& kmer_mapper_;

	bool FindKmer(Kmer kmer, size_t kmer_pos, vector<EdgeId> &passed,
			RangeMappings& range_mappings) const {
		if (index_.containsInIndex(kmer)) {
			pair<EdgeId, size_t> position = index_.get(kmer);
			if (passed.empty() || passed.back() != position.first
					|| kmer_pos != range_mappings.back().initial_range.end_pos
					|| position.second + 1
							< range_mappings.back().mapped_range.end_pos) {
				passed.push_back(position.first);
				range_mappings.push_back(
						MappingRange(Range(kmer_pos, kmer_pos + 1),
								Range(position.second, position.second + 1)));
			} else {
				range_mappings.back().initial_range.end_pos = kmer_pos + 1;
				range_mappings.back().mapped_range.end_pos = position.second
						+ 1;
			}
			return true;
		}
		return false;
	}

	bool TryThread(const Kmer& kmer, size_t kmer_pos, vector<EdgeId> &passed,
			RangeMappings& range_mappings) const {
		EdgeId last_edge = passed.back();
		size_t end_pos = range_mappings.back().mapped_range.end_pos;
		if (end_pos < g_.length(last_edge)) {
			if (g_.EdgeNucls(last_edge)[end_pos + k - 1] == kmer[k - 1]) {
				range_mappings.back().initial_range.end_pos++;
				range_mappings.back().mapped_range.end_pos++;
				return true;
			}
		} else {
			vector<EdgeId> edges = g_.OutgoingEdges(g_.EdgeEnd(last_edge));
			for (size_t i = 0; i < edges.size(); i++) {
				if (g_.EdgeNucls(edges[i])[k - 1] == kmer[k - 1]) {
					passed.push_back(edges[i]);
					range_mappings.push_back(
							MappingRange(Range(kmer_pos, kmer_pos + 1),
									Range(0, 1)));
					return true;
				}
			}
		}
		return false;
	}

	bool Substitute(Kmer& kmer) const {
		Kmer subs = kmer_mapper_.Substitute(kmer);
		if (subs != kmer) {
			kmer = subs;
			return true;
		}
		return false;
	}

	bool ProcessKmer(Kmer kmer, size_t kmer_pos, vector<EdgeId> &passed_edges,
			RangeMappings& range_mapping, bool try_thread) const {
		if (!Substitute(kmer)) {
			if (try_thread) {
				return TryThread(kmer, kmer_pos, passed_edges, range_mapping);
			} else {
				return FindKmer(kmer, kmer_pos, passed_edges, range_mapping);
			}
		} else {
			FindKmer(kmer, kmer_pos, passed_edges, range_mapping);
			return false;
		}
	}

public:
	NewExtendedSequenceMapper(const Graph& g,
			const IdTrackHandler<Graph>& int_ids, const Index& index,
			const KmerSubs& kmer_mapper) :
			g_(g), int_ids_(int_ids), index_(index), kmer_mapper_(kmer_mapper) {
	}

	MappingPath<EdgeId> MapSequence(const Sequence &sequence) const {
		vector<EdgeId> passed_edges;
		RangeMappings range_mapping;

		if (sequence.size() < k) {
			return MappingPath<EdgeId>();
		}

		Kmer kmer = sequence.start<k>() >> 0;
		bool try_thread = false;
		for (size_t i = k - 1; i < sequence.size(); ++i) {
			kmer = kmer << sequence[i];
			try_thread = ProcessKmer(kmer, i - k + 1, passed_edges,
					range_mapping, try_thread);
		}

		//DEBUG
//		for (size_t i = 0; i < passed_edges.size(); ++i) {
//			cerr << int_ids_.ReturnIntId(passed_edges[i]) << " (" << range_mapping[i] << ")"<< "; ";
//		}
//		cerr << endl;
		//DEBUG

		return MappingPath<EdgeId>(passed_edges, range_mapping);
	}
};

template<size_t k, class Graph>
class OldEtalonPairedInfoCounter {
	typedef typename Graph::EdgeId EdgeId;

	const Graph& g_;
	const EdgeIndex<k + 1, Graph>& index_;
	size_t insert_size_;
	size_t read_length_;
	size_t gap_;
	size_t delta_;

	void AddEtalonInfo(omnigraph::PairedInfoIndex<Graph>& paired_info,
			EdgeId e1, EdgeId e2, double d) {
		PairInfo<EdgeId> pair_info(e1, e2, d, 1000.0, 0.);
		paired_info.AddPairInfo(pair_info);
	}

	void ProcessSequence(const Sequence& sequence,
			omnigraph::PairedInfoIndex<Graph>& paired_info) {
		SimpleSequenceMapper<k + 1, Graph> sequence_mapper(g_, index_);
		Path<EdgeId> path = sequence_mapper.MapSequence(sequence);

		for (size_t i = 0; i < path.size(); ++i) {
			EdgeId e = path[i];
			if (g_.length(e) + delta_ > gap_ + k + 1) {
				AddEtalonInfo(paired_info, e, e, 0);
			}
			size_t j = i + 1;
			size_t length = 0;

			while (j < path.size()
					&& length
							<= omnigraph::PairInfoPathLengthUpperBound(k,
									insert_size_, delta_)) {
				if (length
						>= omnigraph::PairInfoPathLengthLowerBound(k,
								g_.length(e), g_.length(path[j]), gap_, delta_)) {
					AddEtalonInfo(paired_info, e, path[j],
							g_.length(e) + length);
				}
				length += g_.length(path[j++]);
			}
		}

	}

	/* DEBUG method
	 void CheckPairInfo(const Sequence& genome, omnigraph::PairedInfoIndex<Graph>& paired_info) {
	 SimpleSequenceMapper<k + 1, Graph> mapper(g_, index_);
	 Path<EdgeId> path = mapper.MapSequence(genome);
	 vector<EdgeId> sequence = path.sequence();
	 EdgeId prev = 0;
	 for (auto it = sequence.begin(); it != sequence.end(); ++it) {
	 if (prev != 0) {
	 vector<PairInfo<EdgeId> > infos = paired_info.GetEdgePairInfo(prev, *it);
	 bool imperfect_flag = false;
	 bool perfect_flag = false;
	 for (auto info_it = infos.begin(); info_it != infos.end(); info_it++) {
	 if (abs((*info_it).d - g_.length(prev)) < 2) {
	 imperfect_flag = true;
	 }
	 if (math::eq((*info_it).d, 0. + g_.length(prev))) {
	 perfect_flag = true;
	 break;
	 }
	 }
	 if (!perfect_flag && imperfect_flag) {
	 cerr<< "AAAAAAAAAAAAAAA" <<endl;
	 }
	 }
	 prev = *it;
	 }
	 }*/

public:

	OldEtalonPairedInfoCounter(const Graph& g,
			const EdgeIndex<k + 1, Graph>& index, size_t insert_size,
			size_t read_length, size_t delta) :
			g_(g), index_(index), insert_size_(insert_size), read_length_(
					read_length), gap_(insert_size_ - 2 * read_length_), delta_(
					delta) {
		VERIFY(insert_size_ >= 2 * read_length_);
	}

	void FillEtalonPairedInfo(const Sequence& genome,
			omnigraph::PairedInfoIndex<Graph>& paired_info) {
		ProcessSequence(genome, paired_info);
		ProcessSequence(!genome, paired_info);
		//DEBUG
		//		CheckPairInfo(genome, paired_info);
	}
};

template<size_t k, class Graph>
class EtalonPairedInfoCounter {
	typedef typename Graph::EdgeId EdgeId;

	const Graph& g_;
	const EdgeIndex<k + 1, Graph>& index_;
	const KmerMapper<k + 1, Graph>& kmer_mapper_;
	size_t insert_size_;
	size_t read_length_;
	size_t gap_;
	size_t delta_;

	void AddEtalonInfo(set<PairInfo<EdgeId>>& paired_info, EdgeId e1, EdgeId e2,
			double d) {
		PairInfo<EdgeId> pair_info(e1, e2, d, 1000.0, 0.);
		paired_info.insert(pair_info);
	}

	void ProcessSequence(const Sequence& sequence,
			set<PairInfo<EdgeId>>& temporary_info) {
		int mod_gap = (gap_ > delta_) ? gap_ - delta_ : 0;
		Seq<k + 1> left(sequence);
		left = left >> 0;
		for (size_t left_idx = 0;
				left_idx + 2 * (k + 1) + mod_gap <= sequence.size();
				++left_idx) {
			left = left << sequence[left_idx + k];
			Seq<k + 1> left_upd = kmer_mapper_.Substitute(left);
			if (!index_.containsInIndex(left_upd)) {
				continue;
			}
			pair<EdgeId, size_t> left_pos = index_.get(left_upd);

			size_t right_idx = left_idx + k + 1 + mod_gap;
			Seq<k + 1> right(sequence, right_idx);
			right = right >> 0;
			for (;
					right_idx + k + 1 <= left_idx + insert_size_ + delta_
							&& right_idx + k + 1 <= sequence.size();
					++right_idx) {
				right = right << sequence[right_idx + k];
				Seq<k + 1> right_upd = kmer_mapper_.Substitute(right);
				if (!index_.containsInIndex(right_upd)) {
					continue;
				}
				pair<EdgeId, size_t> right_pos = index_.get(right_upd);

				AddEtalonInfo(
						temporary_info,
						left_pos.first,
						right_pos.first,
						0. + right_idx - left_idx + left_pos.second
								- right_pos.second);
			}
		}
	}

public:

	EtalonPairedInfoCounter(const Graph& g, const EdgeIndex<k + 1, Graph>& index
			,
			const KmerMapper<k + 1, Graph>& kmer_mapper,
			size_t insert_size,
			size_t read_length, size_t delta) :
			g_(g), index_(index), kmer_mapper_(kmer_mapper), insert_size_(
					insert_size), read_length_(read_length), gap_(
					insert_size_ - 2 * read_length_), delta_(delta) {
		VERIFY(insert_size_ >= 2 * read_length_);
	}

	void FillEtalonPairedInfo(const Sequence& genome,
			omnigraph::PairedInfoIndex<Graph>& paired_info) {
		set<PairInfo<EdgeId>> temporary_info;
		ProcessSequence(genome, temporary_info);
		ProcessSequence(!genome, temporary_info);
		for (auto it = temporary_info.begin(); it != temporary_info.end();
				++it) {
			paired_info.AddPairInfo(*it);
		}
	}
};

double PairedReadCountWeight(const MappingRange&, const MappingRange&) {
	return 1.;
}

double KmerCountProductWeight(const MappingRange& mr1,
		const MappingRange& mr2) {
	return mr1.initial_range.size() * mr2.initial_range.size();
}

ConjugateDeBruijnGraph::EdgeId conj_wrap(ConjugateDeBruijnGraph& g,
		ConjugateDeBruijnGraph::EdgeId e) {
	return g.conjugate(e);
}

NonconjugateDeBruijnGraph::EdgeId conj_wrap(NonconjugateDeBruijnGraph& g,
		NonconjugateDeBruijnGraph::EdgeId e) {
	VERIFY(0);
	return e;
}

void WrappedSetCoverage(ConjugateDeBruijnGraph& g,
		ConjugateDeBruijnGraph::EdgeId e, int cov) {
		g.coverage_index().SetCoverage(e, cov);
		g.coverage_index().SetCoverage(g.conjugate(e), cov);
}

void WrappedSetCoverage(NonconjugateDeBruijnGraph& g,
		NonconjugateDeBruijnGraph::EdgeId e, int cov) {
	g.coverage_index().SetCoverage(e, cov);
}

/**
 * As for now it ignores sophisticated case of repeated consecutive
 * occurrence of edge in path due to gaps in mapping
 *
 * todo talk with Anton about simplification and speed-up of procedure with little quality loss
 */
template<size_t k, class Graph, class SequenceMapper, class Stream>
class LatePairedIndexFiller {
private:
	typedef typename Graph::EdgeId EdgeId;
	typedef Seq<k> Kmer;
	typedef boost::function<double(MappingRange, MappingRange)> WeightF;
	const Graph& graph_;
	const SequenceMapper mapper_;
	Stream& stream_;
	WeightF weight_f_;

	void ProcessPairedRead(omnigraph::PairedInfoIndex<Graph>& paired_index,
			const io::PairedRead& p_r) {
		//DEBUG
		//static size_t count = 0;
		//DEBUG

		Sequence read1 = p_r.first().sequence();
		Sequence read2 = p_r.second().sequence();

		MappingPath<EdgeId> path1 = mapper_.MapSequence(read1);
		//		cout << "Path1 length " << path1.size() << endl;
		MappingPath<EdgeId> path2 = mapper_.MapSequence(read2);
		//		cout << "Path2 length " << path2.size() << endl;
		size_t read_distance = p_r.distance();
		for (size_t i = 0; i < path1.size(); ++i) {
			pair<EdgeId, MappingRange> mapping_edge_1 = path1[i];
			for (size_t j = 0; j < path2.size(); ++j) {
				pair<EdgeId, MappingRange> mapping_edge_2 = path2[j];
				double weight = weight_f_(mapping_edge_1.second,
						mapping_edge_2.second);
				size_t kmer_distance = read_distance
						+ mapping_edge_2.second.initial_range.start_pos
						- mapping_edge_1.second.initial_range.start_pos;
				int edge_distance = kmer_distance
						+ mapping_edge_1.second.mapped_range.start_pos
						- mapping_edge_2.second.mapped_range.start_pos;

				paired_index.AddPairInfo(
						PairInfo<EdgeId>(mapping_edge_1.first,
								mapping_edge_2.first, (double) edge_distance,
								weight, 0.));
				//DEBUG
//				cout << "here2 " << PairInfo<EdgeId> (mapping_edge_1.first,
//						mapping_edge_2.first, (double) edge_distance, weight,
//						0.) << endl;
//				count++;
//				if (count == 10000) {
//					exit(0);
//				}
				//DEBUG
			}
		}
	}

public:

	LatePairedIndexFiller(const Graph &graph, const SequenceMapper& mapper,
			Stream& stream, WeightF weight_f) :
			graph_(graph), mapper_(mapper), stream_(stream), weight_f_(weight_f) {

	}

	void FillIndex(omnigraph::PairedInfoIndex<Graph>& paired_index) {
		for (auto it = graph_.SmartEdgeBegin(); !it.IsEnd(); ++it) {
			//			cout << "here1" << endl;
			paired_index.AddPairInfo(PairInfo<EdgeId>(*it, *it, 0, 0.0, 0.));
		}
		stream_.reset();
		while (!stream_.eof()) {
			io::PairedRead p_r;
			stream_ >> p_r;
			ProcessPairedRead(paired_index, p_r);
		}
	}

};

/**
 * This class finds how certain _paired_ read is mapped to genome. As it is now it is hoped to work correctly only if read
 * is mapped to graph ideally and in unique way.
 */
template<size_t k, class Graph, class Stream>
class TemplateReadMapper {
public:
	typedef typename Graph::EdgeId EdgeId;
	typedef EdgeIndex<k + 1, Graph> Index;
private:
	SimpleSequenceMapper<k, Graph> read_seq_mapper;
	Stream& stream_;
public:
	/**
	 * Creates TemplateReadMapper for given graph. Also requires index_ which should be synchronized
	 * with graph.
	 * @param g graph sequences should be mapped to
	 * @param index index syncronized with graph
	 */
	TemplateReadMapper(const Graph& g, const Index& index, Stream & stream) :
			read_seq_mapper(g, index), stream_(stream) {
		stream_.reset();
	}

	/*	ReadThreaderResult<k + 1, Graph> ThreadNext() {
	 if (!stream_.eof()) {
	 io::PairedRead p_r;
	 stream_ >> p_r;
	 Sequence read1 = p_r.first().sequence();
	 Sequence read2 = p_r.second().sequence();
	 Path<EdgeId> aligned_read[2];
	 aligned_read[0] = read_seq_mapper.MapSequence(read1);
	 aligned_read[1] = read_seq_mapper.MapSequence(read2);
	 size_t insert_size = p_r.insert_size();
	 // TODO bug here with insert_size/distance
	 int current_distance1 = insert_size + aligned_read[0].start_pos() - aligned_read[1].start_pos();
	 return ReadThreaderResult<k + 1, Graph> (aligned_read[0],
	 aligned_read[1], current_distance1);
	 }
	 //		else return NULL;
	 }
	 */
};

template<size_t k, class Graph>
class SingleReadMapper {
public:
	typedef typename Graph::EdgeId EdgeId;
	typedef EdgeIndex<k + 1, Graph> Index;
private:
	SimpleSequenceMapper<k + 1, Graph> read_seq_mapper;
	const Graph& g_;
	const Index& index_;
public:
	/**
	 * Creates SingleReadMapper for given graph. Also requires index_ which should be synchronized
	 * with graph.
	 * @param g graph sequences should be mapped to
	 * @param index index syncronized with graph
	 */
	SingleReadMapper(const Graph& g, const Index& index) :
			read_seq_mapper(g, index), g_(g), index_(index) {
	}

	vector<EdgeId> GetContainingEdges(io::SingleRead& p_r) {
		vector<EdgeId> res;

		Sequence read = p_r.sequence();
		if (k + 1 <= read.size()) {
			Seq<k + 1> kmer = read.start<k + 1>();
			bool found;
			for (size_t i = k + 1; i <= read.size(); ++i) {
				if (index_.containsInIndex(kmer)) {
					pair<EdgeId, size_t> position = index_.get(kmer);
					found = false;
					for (size_t j = 0; j < res.size(); j++)
						if (res[j] == position.first) {
							found = true;
							break;
						}
					if (!found)
						res.push_back(position.first);
				}
				if (i != read.size())
					kmer = kmer << read[i];
			}
		}

		return res;
	}
};

template<class Graph>
class EdgeQuality: public GraphLabeler<Graph>, public GraphActionHandler<Graph> {
	typedef typename Graph::EdgeId EdgeId;
	typedef typename Graph::VertexId VertexId;
	map<EdgeId, size_t> quality_;

public:
	template<size_t l>
	void FillQuality(const EdgeIndex<l, Graph> &index
			, const KmerMapper<l, Graph>& kmer_mapper, const Sequence &genome) {
		if (genome.size() < l)
			return;
		auto cur = genome.start<l>();
		cur = cur >> 0;
		for (size_t i = 0; i + l - 1 < genome.size(); i++) {
			cur = cur << genome[i + l - 1];
			auto corr_cur = kmer_mapper.Substitute(cur);
			if (index.containsInIndex(corr_cur)) {
				quality_[index.get(corr_cur).first]++;
			}
		}
	}

	template<size_t l>
	EdgeQuality(const Graph &graph, const EdgeIndex<l, Graph> &index,
	const KmerMapper<l, Graph>& kmer_mapper,
	const Sequence &genome) :
			GraphActionHandler<Graph>(graph, "EdgeQualityLabeler") {
		FillQuality(index, kmer_mapper, genome);
		FillQuality(index, kmer_mapper, !genome);
	}

	virtual ~EdgeQuality() {
	}

	virtual void HandleAdd(EdgeId e) {
	}

	virtual void HandleDelete(EdgeId e) {
		quality_.erase(e);
	}

	virtual void HandleMerge(vector<EdgeId> old_edges, EdgeId new_edge) {
		size_t res = 0;
		for (size_t i = 0; i < old_edges.size(); i++) {
			res += quality_[old_edges[i]];
		}
		quality_[new_edge] += res;
	}

	virtual void HandleGlue(EdgeId new_edge, EdgeId edge1, EdgeId edge2) {
		quality_[new_edge] += quality_[edge1];
		quality_[new_edge] += quality_[edge2];
	}

	virtual void HandleSplit(EdgeId old_edge, EdgeId new_edge1,
			EdgeId new_edge2) {
		quality_[new_edge1] = quality_[old_edge] * this->g().length(new_edge1)
				/ (this->g().length(new_edge1) + this->g().length(new_edge2));
		quality_[new_edge2] = quality_[old_edge] * this->g().length(new_edge2)
				/ (this->g().length(new_edge1) + this->g().length(new_edge2));
	}

	double quality(EdgeId edge) const {
		auto it = quality_.find(edge);
		if (it == quality_.end())
			return 0.;
		else
			return 1. * it->second / this->g().length(edge);
	}

	bool IsPositiveQuality(EdgeId edge) const {
		return math::gr(quality(edge), 0.);
	}

	virtual std::string label(VertexId vertexId) const {
		return "";
	}

	virtual std::string label(EdgeId edge) const {
		double q = quality(edge);
		return (q == 0) ? "" : ToString("quality: " << q);
	}

};

template<class Graph>
class QualityLoggingRemovalHandler {
	typedef typename Graph::EdgeId EdgeId;
	const EdgeQuality<Graph>& quality_handler_;
//	size_t black_removed_;
//	size_t colored_removed_;
public:
	QualityLoggingRemovalHandler(const EdgeQuality<Graph>& quality_handler) :
			quality_handler_(quality_handler)/*, black_removed_(0), colored_removed_(
	 0)*/{

	}

	void HandleDelete(EdgeId edge) {
		TRACE("Deleting edge with quality " << quality_handler_.quality(edge));
//		if (math::gr(quality_handler_.quality(edge), 0.))
//			colored_removed_++;
//		else
//			black_removed_++;
	}

private:
	DECL_LOGGER("QualityLoggingRemovalHandler")
	;
};

template<class Graph, size_t k>
class KMerNeighborhoodFinder: public omnigraph::GraphSplitter<Graph> {
private:
	typedef typename Graph::EdgeId EdgeId;
	typedef typename Graph::VertexId VertexId;
	EdgeIndex<k + 1, Graph> &index_;
	Seq<k + 1> kp1mer_;
	size_t max_size_;
	size_t edge_length_bound_;
	bool finished_;
public:
	KMerNeighborhoodFinder(const Graph &graph, Seq<k + 1> kp1mer,
			EdgeIndex<k + 1, Graph> &index , size_t max_size
			, size_t edge_length_bound) :
			GraphSplitter<Graph>(graph), index_(index), kp1mer_(kp1mer), max_size_(
					max_size), edge_length_bound_(edge_length_bound), finished_(
					false) {
	}

	virtual ~KMerNeighborhoodFinder() {
	}

	virtual vector<VertexId> NextComponent() {
		CountingDijkstra<Graph> cf(this->graph(), max_size_,
				edge_length_bound_);
		EdgeId edge = index_.get(kp1mer_).first;
		set<VertexId> result_set;
		cf.run(this->graph().EdgeStart(edge));
		vector<VertexId> result_start = cf.VisitedVertices();
		result_set.insert(result_start.begin(), result_start.end());
		cf.run(this->graph().EdgeEnd(edge));
		vector<VertexId> result_end = cf.VisitedVertices();
		result_set.insert(result_end.begin(), result_end.end());
		finished_ = true;
		vector<VertexId> result;
		for (auto it = result_set.begin(); it != result_set.end(); ++it)
			result.push_back(*it);
		return result;
	}

	virtual bool Finished() {
		return finished_;
	}
};

}
