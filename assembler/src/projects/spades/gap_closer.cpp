//***************************************************************************
//* Copyright (c) 2015 Saint Petersburg State University
//* Copyright (c) 2011-2014 Saint Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#include "gap_closer.hpp"
#include "assembly_graph/stats/picture_dump.hpp"
#include "modules/simplification/compressor.hpp"
#include "modules/alignment/sequence_mapper_notifier.hpp"
#include "paired_info/concurrent_pair_info_buffer.hpp"
#include "io/dataset_support/read_converter.hpp"
#include <stack>

namespace debruijn_graph {

class GapCloserPairedIndexFiller : public SequenceMapperListener {
private:
    const Graph &graph_;
    typedef std::unordered_map<EdgeId, std::pair<EdgeId, int>> TipMap;
    omnigraph::de::PairedInfoIndexT<Graph> &paired_index_;
    omnigraph::de::ConcurrentPairedInfoBuffer<Graph> buffer_pi_;
    TipMap out_tip_map_, in_tip_map_;

    size_t CorrectLength(Path<EdgeId> path, size_t idx) const {
        size_t answer = graph_.length(path[idx]);
        if (idx == 0)
            answer -= path.start_pos();
        if (idx == path.size() - 1)
            answer -= graph_.length(path[idx]) - path.end_pos();
        return answer;
    }

    void ProcessPairedRead(const MappingPath<EdgeId> &path1, const MappingPath<EdgeId> &path2) {
        for (size_t i = 0; i < path1.size(); ++i) {
            auto OutTipIter = out_tip_map_.find(path1[i].first);
            if (OutTipIter == out_tip_map_.cend()) {
                continue;
            }

            for (size_t j = 0; j < path2.size(); ++j) {
                auto InTipIter = in_tip_map_.find(path2[j].first);
                if (InTipIter == in_tip_map_.cend()) {
                    continue;
                }

                auto e1 = OutTipIter->second.first;
                auto e2 = InTipIter->second.first;
                //FIXME: Normalize fake points
                auto sp = std::make_pair(e1, e2);
                auto cp = buffer_pi_.ConjugatePair(e1, e2);
                auto ip = std::min(sp, cp);
                buffer_pi_.Add(ip.first, ip.second, omnigraph::de::RawPoint(1000000., 1.));
            }
        }
    }

    void PrepareShiftMaps(TipMap &OutTipMap, TipMap &InTipMap) const {
        size_t nthreads = omp_get_max_threads();

        omnigraph::IterationHelper<Graph, EdgeId> edges(graph_);
        auto iters = edges.Chunks(nthreads);
        VERIFY(iters.size() == nthreads + 1);
#pragma omp parallel for
        for (size_t i = 0; i < nthreads; ++i) {
            TipMap local_in_tip_map, local_out_tip_map;
            for (EdgeId edge : adt::make_range(iters[i], iters[i + 1])) {
                if (graph_.IsDeadStart(graph_.EdgeStart(edge))) {
                    local_in_tip_map.insert({edge, {edge, 0}});
                    std::stack<std::pair<EdgeId, int>> edge_stack;
                    edge_stack.push({edge, 0});
                    while (!edge_stack.empty()) {
                        auto checking_pair = edge_stack.top();
                        edge_stack.pop();
                        if (graph_.CheckUniqueIncomingEdge(graph_.EdgeEnd(checking_pair.first))) {
                            for (EdgeId e : graph_.OutgoingEdges(graph_.EdgeEnd(checking_pair.first))) {
                                local_in_tip_map.insert({e, {edge, graph_.length(checking_pair.first) +
                                                        checking_pair.second}});
                                edge_stack.push({e, graph_.length(checking_pair.first) + checking_pair.second});

                            }
                        }
                    }
                }

                if (graph_.IsDeadEnd(graph_.EdgeEnd(edge))) {
                    local_out_tip_map.insert({edge, {edge, 0}});
                    std::stack<std::pair<EdgeId, int>> edge_stack;
                    edge_stack.push({edge, 0});
                    while (!edge_stack.empty()) {
                        auto checking_pair = edge_stack.top();
                        edge_stack.pop();
                        if (graph_.CheckUniqueOutgoingEdge(graph_.EdgeStart(checking_pair.first))) {
                            for (EdgeId e : graph_.IncomingEdges(graph_.EdgeStart(checking_pair.first))) {
                                local_out_tip_map.insert({e, {edge, graph_.length(e) + checking_pair.second}});
                                edge_stack.push({e, graph_.length(e) + checking_pair.second});
                            }
                        }

                    }
                }
            }

#pragma omp critical
            {
                InTipMap.insert(std::make_move_iterator(local_in_tip_map.begin()), std::make_move_iterator(local_in_tip_map.end()));
            }
#pragma omp critical
            {
                OutTipMap.insert(std::make_move_iterator(local_out_tip_map.begin()), std::make_move_iterator(local_out_tip_map.end()));
            }
        }
    }

public:
    GapCloserPairedIndexFiller(const Graph &graph, omnigraph::de::PairedInfoIndexT<Graph> &paired_index)
            : graph_(graph), paired_index_(paired_index), buffer_pi_(graph) { }

    void StartProcessLibrary(size_t /* threads_count */) override {
        paired_index_.clear();
        INFO("Preparing shift maps");
        PrepareShiftMaps(out_tip_map_, in_tip_map_);
    }

    void StopProcessLibrary() override {
        paired_index_.Merge(buffer_pi_);
        buffer_pi_.clear();
        out_tip_map_.clear();
        in_tip_map_.clear();
    }

    void ProcessPairedRead(size_t /* thread_index */, const io::PairedRead&  /* pr */,
                           const MappingPath<EdgeId> &path1,
                           const MappingPath<EdgeId> &path2) override {
        ProcessPairedRead(path1, path2);
    }
    void ProcessPairedRead(size_t /* thread_index */, const io::PairedReadSeq& /* pr */,
                           const MappingPath<EdgeId> &path1,
                           const MappingPath<EdgeId> &path2) override {
        ProcessPairedRead(path1, path2);
    }
};

class GapCloser {
    typedef typename Graph::EdgeId EdgeId;
    typedef typename Graph::VertexId VertexId;
    typedef std::vector<size_t> MismatchPos;

    Graph &g_;
    int k_;
    omnigraph::de::PairedInfoIndexT<Graph> &tips_paired_idx_;
    const size_t min_intersection_;
    const size_t hamming_dist_bound_;
    const omnigraph::de::DEWeight weight_threshold_;

    std::vector<size_t> DiffPos(const Sequence &s1, const Sequence &s2) const {
        VERIFY(s1.size() == s2.size());
        std::vector<size_t> answer;
        for (size_t i = 0; i < s1.size(); ++i)
            if (s1[i] != s2[i])
                answer.push_back(i);
        return answer;
    }

    size_t HammingDistance(const Sequence &s1, const Sequence &s2) const {
        VERIFY(s1.size() == s2.size());
        size_t dist = 0;
        for (size_t i = 0; i < s1.size(); ++i)
            if (s1[i] != s2[i])
                dist++;
        return dist;
    }

    //  size_t HammingDistance(const Sequence& s1, const Sequence& s2) const {
    //    return DiffPos(s1, s2).size();
    //  }

    std::vector<size_t> PosThatCanCorrect(size_t overlap_length/*in nucls*/, const MismatchPos &mismatch_pos,
                                          size_t edge_length/*in nucls*/, bool left_edge) const {
        TRACE("Try correct left edge " << left_edge);
        TRACE("Overlap length " << overlap_length);
        TRACE("Edge length " << edge_length);
        TRACE("Mismatches " << mismatch_pos);

        std::vector<size_t> answer;
        for (size_t i = 0; i < mismatch_pos.size(); ++i) {
            size_t relative_mm_pos =
                    left_edge ?
                    mismatch_pos[i] :
                    overlap_length - 1 - mismatch_pos[i];
            if (overlap_length - relative_mm_pos + g_.k() < edge_length)
                //can correct mismatch
                answer.push_back(mismatch_pos[i]);
        }
        TRACE("Can correct mismatches: " << answer);
        return answer;
    }

    //todo write easier
    bool CanCorrectLeft(EdgeId e, int overlap, const MismatchPos &mismatch_pos) const {
        return PosThatCanCorrect(overlap, mismatch_pos, g_.length(e) + g_.k(), true).size() == mismatch_pos.size();
    }

    //todo write easier
    bool CanCorrectRight(EdgeId e, int overlap, const MismatchPos &mismatch_pos) const {
        return PosThatCanCorrect(overlap, mismatch_pos, g_.length(e) + g_.k(), false).size() == mismatch_pos.size();
    }

    bool MatchesEnd(const Sequence &long_seq, const Sequence &short_seq, bool from_begin) const {
        return from_begin ? long_seq.First(short_seq.size()) == short_seq
                          : long_seq.Last(short_seq.size()) == short_seq;
    }

    void CorrectLeft(EdgeId first, EdgeId second, int overlap, const MismatchPos &diff_pos) {
        DEBUG("Can correct first with sequence from second.");
        Sequence new_sequence = g_.EdgeNucls(first).Subseq(g_.length(first) - overlap + diff_pos.front(),
                                                           g_.length(first) + k_ - overlap)
                                + g_.EdgeNucls(second).First(k_);
        DEBUG("Checking new k+1-mers.");
        DEBUG("Check ok.");
        DEBUG("Splitting first edge.");
        auto split_res = g_.SplitEdge(first, g_.length(first) - overlap + diff_pos.front());
        first = split_res.first;
        tips_paired_idx_.Remove(split_res.second);
        DEBUG("Adding new edge.");
        VERIFY(MatchesEnd(new_sequence, g_.VertexNucls(g_.EdgeEnd(first)), true));
        VERIFY(MatchesEnd(new_sequence, g_.VertexNucls(g_.EdgeStart(second)), false));
        g_.AddEdge(g_.EdgeEnd(first), g_.EdgeStart(second),
                new_sequence);
    }

    void CorrectRight(EdgeId first, EdgeId second, int overlap, const MismatchPos &diff_pos) {
        DEBUG("Can correct second with sequence from first.");
        Sequence new_sequence =
                g_.EdgeNucls(first).Last(k_) + g_.EdgeNucls(second).Subseq(overlap, diff_pos.back() + 1 + k_);
        DEBUG("Checking new k+1-mers.");
        DEBUG("Check ok.");
        DEBUG("Splitting second edge.");
        auto split_res = g_.SplitEdge(second, diff_pos.back() + 1);
        second = split_res.second;
        tips_paired_idx_.Remove(split_res.first);
        DEBUG("Adding new edge.");
        VERIFY(MatchesEnd(new_sequence, g_.VertexNucls(g_.EdgeEnd(first)), true));
        VERIFY(MatchesEnd(new_sequence, g_.VertexNucls(g_.EdgeStart(second)), false));

        g_.AddEdge(g_.EdgeEnd(first), g_.EdgeStart(second),
                new_sequence);
    }

    bool HandlePositiveHammingDistanceCase(EdgeId first, EdgeId second, int overlap) {
        DEBUG("Match was imperfect. Trying to correct one of the tips");
        auto diff_pos = DiffPos(g_.EdgeNucls(first).Last(overlap), g_.EdgeNucls(second).First(overlap));
        if (CanCorrectLeft(first, overlap, diff_pos)) {
            CorrectLeft(first, second, overlap, diff_pos);
            return true;
        } else if (CanCorrectRight(second, overlap, diff_pos)) {
            CorrectRight(first, second, overlap, diff_pos);
            return true;
        } else {
            DEBUG("Can't correct tips due to the graph structure");
            return false;
        }
    }

    bool HandleSimpleCase(EdgeId first, EdgeId second, int overlap) {
        DEBUG("Match was perfect. No correction needed");
        DEBUG("Overlap " << overlap);
        //strange info guard
        VERIFY(overlap <= k_);
        if (overlap == k_) {
            DEBUG("Tried to close zero gap");
            return false;
        }
        //old code
        Sequence edge_sequence = g_.EdgeNucls(first).Last(k_)
                                 + g_.EdgeNucls(second).Subseq(overlap, k_);
        DEBUG("Gap filled: Gap size = " << k_ - overlap << "  Result seq "
              << edge_sequence.str());
        g_.AddEdge(g_.EdgeEnd(first), g_.EdgeStart(second), edge_sequence);
        return true;
    }

    bool ProcessPair(EdgeId first, EdgeId second) {
        TRACE("Processing edges " << g_.str(first) << " and " << g_.str(second));
        TRACE("first " << g_.EdgeNucls(first) << " second " << g_.EdgeNucls(second));

        if (cfg::get().avoid_rc_connections &&
            (first == g_.conjugate(second) || first == second)) {
            DEBUG("Trying to join conjugate edges " << g_.int_id(first));
            return false;
        }

        Sequence seq1 = g_.EdgeNucls(first);
        Sequence seq2 = g_.EdgeNucls(second);
        TRACE("Checking possible gaps from 1 to " << k_ - min_intersection_);
        for (int gap = 1; gap <= k_ - (int) min_intersection_; ++gap) {
            int overlap = k_ - gap;
            size_t hamming_distance = HammingDistance(g_.EdgeNucls(first).Last(overlap),
                                                      g_.EdgeNucls(second).First(overlap));
            if (hamming_distance <= hamming_dist_bound_) {
                DEBUG("For edges " << g_.str(first) << " and " << g_.str(second)
                      << ". For gap value " << gap << " (overlap " << overlap << "bp) hamming distance was " <<
                      hamming_distance);
                //        DEBUG("Sequences of distance " << tip_distance << " :"
                //                << seq1.Subseq(seq1.size() - k).str() << "  "
                //                << seq2.Subseq(0, k).str());

                if (hamming_distance > 0) {
                    return HandlePositiveHammingDistanceCase(first, second, overlap);
                } else {
                    return HandleSimpleCase(first, second, overlap);
                }
            }
        }
        return false;
    }

public:
    //TODO extract methods
    void CloseShortGaps() {
        INFO("Closing short gaps");
        size_t gaps_filled = 0;
        size_t gaps_checked = 0;
        for (auto edge = g_.SmartEdgeBegin(); !edge.IsEnd(); ++edge) {
            EdgeId first_edge = *edge;
            for (auto i : tips_paired_idx_.Get(first_edge)) {
                EdgeId second_edge = i.first;
                if (first_edge == second_edge)
                    continue;

                if (!g_.IsDeadEnd(g_.EdgeEnd(first_edge)) || !g_.IsDeadStart(g_.EdgeStart(second_edge))) {
                    // WARN("Topologically wrong tips");
                    continue;
                }

                bool closed = false;
                for (auto point : i.second) {
                    if (math::ls(point.weight, weight_threshold_))
                        continue;

                    ++gaps_checked;
                    closed = ProcessPair(first_edge, second_edge);
                    if (closed) {
                        ++gaps_filled;
                        break;
                    }
                }
                if (closed)
                    break;
            } // second edge
        } // first edge

        INFO("Closing short gaps complete: filled " << gaps_filled
             << " gaps after checking " << gaps_checked
             << " candidates");
        omnigraph::CompressAllVertices<Graph>(g_);
    }

    GapCloser(Graph &g, omnigraph::de::PairedInfoIndexT<Graph> &tips_paired_idx,
              size_t min_intersection, double weight_threshold,
              size_t hamming_dist_bound = 0 /*min_intersection_ / 5*/)
            : g_(g),
              k_((int) g_.k()),
              tips_paired_idx_(tips_paired_idx),
              min_intersection_(min_intersection),
              hamming_dist_bound_(hamming_dist_bound),
              weight_threshold_(weight_threshold)  {
        VERIFY(min_intersection_ < g_.k());
        DEBUG("weight_threshold=" << weight_threshold_);
        DEBUG("min_intersect=" << min_intersection_);
        DEBUG("paired_index size=" << tips_paired_idx_.size());
    }

private:
    DECL_LOGGER("GapCloser");
};

void GapClosing::run(GraphPack &gp, const char *) {
    visualization::graph_labeler::DefaultLabeler<Graph> labeler(gp.get<Graph>(), gp.get<EdgesPositionHandler<Graph>>());
    stats::detail_info_printer printer(gp, labeler, cfg::get().output_dir);
    printer(config::info_printer_pos::before_first_gap_closer);

    bool pe_exist = false;
    for (const auto& lib : cfg::get().ds.reads.libraries()) {
        if (lib.type() != io::LibraryType::PairedEnd)
            continue;

        pe_exist = true;
    }
    if (!pe_exist) {
        INFO("No paired-end libraries exist, skipping gap closer");
        return;
    }

    gp.EnsureIndex();
    auto &g = gp.get_mutable<Graph>();
    omnigraph::de::PairedInfoIndexT<Graph> tips_paired_idx(g);
    GapCloserPairedIndexFiller gcpif(g, tips_paired_idx);

    SequenceMapperNotifier notifier(gp, cfg::get().ds.reads.lib_count());
    auto mapper = MapperInstance(gp);

    auto& dataset = cfg::get_writable().ds;
    for (size_t i = 0; i < dataset.reads.lib_count(); ++i) {
        if (dataset.reads[i].type() != io::LibraryType::PairedEnd)
            continue;

        notifier.Subscribe(i, &gcpif);
        io::BinaryPairedStreams paired_streams = paired_binary_readers(dataset.reads[i], false, 0, false);
        notifier.ProcessLibrary(paired_streams, i, *mapper);

        INFO("Initializing gap closer");
        GapCloser gap_closer(g, tips_paired_idx,
                             cfg::get().gc.minimal_intersection, cfg::get().gc.weight_threshold);
        gap_closer.CloseShortGaps();
        INFO("Gap closer done");
    }
}

}
