//***************************************************************************
//* Copyright (c) 2015 Saint Petersburg State University
//* Copyright (c) 2011-2014 Saint Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#pragma once

#include <adt/iterator_range.hpp>
#include <btree/safe_btree_map.h>
#include <sparsehash/sparse_hash_map>

#include <type_traits>

#include "index_point.hpp"

namespace omnigraph {

namespace de {

template<typename Histogram, bool back> struct IterHelper;

template<typename Histogram> struct IterHelper<Histogram, false> {
    typedef typename Histogram::const_iterator Iterator;
    static Iterator begin(const Histogram& h) { return h.begin(); }
    static Iterator end(const Histogram& h) { return h.end(); }
};

template<typename Histogram> struct IterHelper<Histogram, true> {
    typedef typename Histogram::const_reverse_iterator Iterator;
    static Iterator begin(const Histogram& h) { return h.rbegin(); }
    static Iterator end(const Histogram& h) { return h.rend(); }
};

/**
 * @brief Paired reads info storage. Arranged as a map of map of info points.
 * @param G graph type
 * @param H map-like container type (parameterized by key and value type)
 */
template<typename G, typename H, template<typename, typename> class Container>
class PairedIndex {

public:
    typedef G Graph;
    typedef H Histogram;
    typedef typename Graph::EdgeId EdgeId;
    typedef std::pair<EdgeId, EdgeId> EdgePair;
    typedef typename Histogram::value_type Point;

    typedef Container<EdgeId, Histogram> InnerMap;
    typedef Container<EdgeId, InnerMap> StorageMap;

    //--Data access types--

    typedef typename StorageMap::const_iterator ImplIterator;

public:
    /**
     * @brief Smart proxy set representing a composite histogram of points between two edges.
     * @param full When true, represents the whole histogram (consisting both of directly added points
     *             and "restored" conjugates).
     *             When false, proxifies only the added points.
     * @detail You can work with the proxy just like with any constant set.
     *         The only major difference is that it returns all consisting points by value,
     *         becauses some of them don't exist in the underlying sets and are
     *         restored from the conjugate info on-the-fly.
     */
    template<bool back = false>
    class HistProxy {

    public:
        /**
         * @brief Iterator over a proxy set of points.
         * @warning Generally, the proxy is unordered even if the set is ordered.
         *          If you require that, convert it into a flat histogram with Unwrap().
         * @param full When true, traverses both straight and conjugate points,
         *             and automatically recalculates the distance for latter.
         *             When false, traverses only the added points and skips the rest.
         */
        class Iterator: public boost::iterator_facade<Iterator, Point, boost::bidirectional_traversal_tag, Point> {

            typedef typename IterHelper<Histogram, back>::Iterator InnerIterator;

        public:
            Iterator(InnerIterator iter)
                    : iter_(iter)
            {}

        private:
            friend class boost::iterator_core_access;

            Point dereference() const {
                Point result = *iter_;
                if (back)
                    result.d = -result.d;
                return result;
            }

            void increment() {
                ++iter_;
            }

            void decrement() {
                --iter_;
            }

            inline bool equal(const Iterator &other) const {
                return iter_ == other.iter_;
            }

            InnerIterator iter_; //current position
        };

        /**
         * @brief Returns a wrapper for a histogram.
         */
        HistProxy(const Histogram& hist)
            : hist_(hist)
        {}

        /**
         * @brief Returns an empty proxy (effectively a Null object pattern).
         */
        static const Histogram& empty_hist() {
            static Histogram res;
            return res;
        }

        Iterator begin() const {
            return IterHelper<Histogram, back>::begin(hist_);
        }

        Iterator end() const {
            return IterHelper<Histogram, back>::end(hist_);
        }
        /**
         * @brief Finds the point with the minimal distance.
         */
        Point min() const {
            VERIFY(!empty());
            return *begin();
        }

        /**
         * @brief Finds the point with the maximal distance.
         */
        Point max() const {
            VERIFY(!empty());
            return *--end();
        }

        /**
         * @brief Returns the copy of all points in a simple histogram.
         */
        Histogram Unwrap() const {
            return Histogram(begin(), end());
        }

        size_t size() const {
            return hist_.size();
        }

        bool empty() const {
            return hist_.empty();
        }

    private:
        const Histogram& hist_;
        float offset_;
    };

    /**
     * @brief Type synonym for straight histogram proxies
     */
    typedef HistProxy<false> FullHistProxy;

    /**
     * @brief Type synonym for reversed histogram proxies
     */
    typedef HistProxy<true> BackHistProxy;

    typedef typename FullHistProxy::Iterator HistIterator;
    typedef typename BackHistProxy::Iterator BackHistIterator;

    //---- Traversing edge neighbours ----

    template<bool back = false>
    using EdgeHist = std::pair<EdgeId, HistProxy<back>>;

    /**
     * @brief A proxy map representing neighbourhood of an edge,
     *        where `Key` is the graph edge ID and `Value` is the proxy histogram.
     * @param half When false, represents all neighbours (consisting both of directly added data
     *             and "restored" conjugates).
     *             When true, proxifies only half of the added edges.
     * @detail You can work with the proxy just like with any constant map.
     *         The only major difference is that it returns all consisting pairs by value,
     *         becauses some of them don't exist in the underlying sets and are
     *         restored from the conjugate info on-the-fly.
     */
    template<bool half = false>
    class EdgeProxy {
    public:

        /**
         * @brief Iterator over a proxy map.
         * @param full When true, traverses both straight and conjugate pairs.
         *             When false, traverses only lesser pairs of edges.
         */
        class Iterator: public boost::iterator_facade<Iterator, EdgeHist<>, boost::forward_traversal_tag, EdgeHist<>> {

            typedef typename InnerMap::const_iterator InnerIterator;

            void Skip() { //For a half iterator, skip conjugate pairs
                while (half && iter_ != stop_ && iter_->first < edge_)
                    ++iter_;
            }

        public:
            Iterator(const PairedIndex &index, InnerIterator iter, InnerIterator stop, EdgeId edge)
                    : index_ (index)
                    , iter_(iter)
                    , stop_(stop)
                    , edge_(edge)
            {
                Skip();
            }

            void increment() {
                ++iter_;
                Skip();
            }

            void operator=(const Iterator &other) {
                //TODO: is this risky without an assertion?
                //We shouldn't reassign iterators from one index onto another
                iter_ = other.iter_;
                stop_ = other.stop_;
                edge_ = other.edge_;
            }

        private:
            friend class boost::iterator_core_access;

            bool equal(const Iterator &other) const {
                return iter_ == other.iter_;
            }

            EdgeHist<> dereference() const {
                const auto& hist = iter_->second;
                return std::make_pair(iter_->first, HistProxy<>(hist));
            }

        private:
            const PairedIndex &index_;
            InnerIterator iter_, stop_;
            EdgeId edge_;
        };

        EdgeProxy(const PairedIndex &index, const InnerMap& map, EdgeId edge)
            : index_(index), map_(map), edge_(edge)
        {}

        Iterator begin() const {
            return Iterator(index_, map_.begin(), map_.end(), edge_);
        }

        Iterator end() const {
            return Iterator(index_, map_.end(), map_.end(), edge_);
        }

        HistProxy<> operator[](EdgeId e2) const {
            return index_.Get(edge_, e2);
        }

        HistProxy<true> GetBack(EdgeId e2) const {
            return index_.GetBack(edge_, e2);
        }

        bool empty() const {
            return map_.empty();
        }

    private:
        const PairedIndex& index_;
        const InnerMap& map_;
        EdgeId edge_;
    };

    typedef typename EdgeProxy<false>::Iterator EdgeIterator;
    typedef typename EdgeProxy<>::Iterator HalfEdgeIterator;

    //--Constructor--

    PairedIndex(const Graph &graph)
        : size_(0), graph_(graph)
    {}

    //--Inserting--
public:
    /**
     * @brief Returns a conjugate pair for two edges.
     */
    inline EdgePair ConjugatePair(EdgeId e1, EdgeId e2) const {
        return std::make_pair(graph_.conjugate(e2), graph_.conjugate(e1));
    }
    /**
     * @brief Returns a conjugate pair for a pair of edges.
     */
    inline EdgePair ConjugatePair(EdgePair ep) const {
        return ConjugatePair(ep.first, ep.second);
    }

private:
    void SwapConj(EdgeId &e1, EdgeId &e2) const {
        auto tmp = e1;
        e1 = graph_.conjugate(e2);
        e2 = graph_.conjugate(tmp);
    }

    void SwapConj(EdgeId &e1, EdgeId &e2, Point &p) const {
        SwapConj(e1, e2);
        p.d += CalcOffset(e1, e2);
    }

    float CalcOffset(EdgeId e1, EdgeId e2) const {
        return float(graph_.length(e1)) - float(graph_.length(e2));
    }

public:
    /**
     * @brief Adds a point between two edges to the index,
     *        merging weights if there's already one with the same distance.
     */
    void Add(EdgeId e1, EdgeId e2, Point point) {
        InsertOrMerge(e1, e2, point);
    }

    /**
     * @brief Adds a whole set of points between two edges to the index.
     */
    template<typename TH>
    void AddMany(EdgeId e1, EdgeId e2, const TH& hist) {
        for (auto point : hist) {
            InsertOrMerge(e1, e2, point);
        }
    }

private:

    void InsertOrMerge(EdgeId e1, EdgeId e2,
                       Point sp) {
        auto& straight = storage_[e1][e2];
        auto si = straight.find(sp);
        if (si != straight.end()) {
            MergeData(straight, si, sp);
            if (!IsSymmetric(e1, e2, sp)) {
                SwapConj(e1, e2, sp);
                auto& conjugate = storage_[e1][e2];
                auto ri = conjugate.find(sp);
                VERIFY(ri != conjugate.end());
                MergeData(conjugate, ri, sp);
            }
        } else {
            InsertPoint(straight, sp);
            if (!IsSymmetric(e1, e2, sp)) {
                SwapConj(e1, e2, sp);
                auto& conjugate = storage_[e1][e2];
                InsertPoint(conjugate, sp);
            }
        }
    }

    static bool IsSymmetric(EdgeId e1, EdgeId e2, Point point) {
        return (e1 == e2) && math::eq(point.d, 0.f);
    }

    // modifying the histogram
    inline void InsertPoint(Histogram& histogram, Point point) {
        histogram.insert(point);
        ++size_;
    }

    void MergeData(Histogram& hist, typename Histogram::iterator to_update, const Point& to_merge) {
        //We can't just modify the existing point, because if variation is non-zero,
        //resulting distance will differ
        //TODO: Use @akorobeynikov's smart merging
        auto to_add = *to_update + to_merge;
        auto after_removed = hist.erase(to_update);
        hist.insert(after_removed, to_add);
    }

public:
    /**
     * @brief Adds a lot of info from another index, using fast merging strategy.
     *        Should be used instead of point-by-point index merge.
     */
    template<class Index>
    void Merge(const Index& index_to_add) {
        auto& base_index = storage_;
        for (auto AddI = index_to_add.data_begin(); AddI != index_to_add.data_end(); ++AddI) {
            EdgeId e1_to_add = AddI->first;
            const auto& map_to_add = AddI->second;
            InnerMap& map_already_exists = base_index[e1_to_add];
            MergeInnerMaps(map_to_add, map_already_exists);
        }
    }

private:
    template<class OtherMap>
    void MergeInnerMaps(const OtherMap& map_to_add,
                        InnerMap& map) {
        for (const auto& to_add : map_to_add) {
            Histogram &hist_exists = map[to_add.first];
            size_ += hist_exists.merge(to_add.second);
        }
    }

public:
    //--Data deleting methods--

    /**
     * @brief Removes the specific entry from the index.
     * @warning Don't use it on unclustered index, because hashmaps require set_deleted_item
     * @return The number of deleted entries (0 if there wasn't such entry)
     */
    size_t Remove(EdgeId e1, EdgeId e2, Point point) {
        auto res = RemoveSingle(e1, e2, point);
        if (!IsSymmetric(e1, e2, point)) {
            SwapConj(e1, e2, point);
            res += RemoveSingle(e1, e2, point);
        }
        return res;
    }

    /**
     * @brief Removes the whole histogram from the index.
     * @warning Don't use it on unclustered index, because hashmaps require set_deleted_item
     * @return The number of deleted entries
     */
    size_t Remove(EdgeId e1, EdgeId e2) {
        auto res = RemoveAll(e1, e2);
        if (e1 != e2) { //TODO: what if self-conjugate?
            SwapConj(e1, e2);
            res += RemoveAll(e1, e2);
        }
        return res;
    }

private:

    //TODO: remove duplicode
    size_t RemoveSingle(EdgeId e1, EdgeId e2, Point point) {
        auto i1 = storage_.find(e1);
        if (i1 == storage_.end())
            return 0;
        auto& map = i1->second;
        auto i2 = map.find(e2);
        if (i2 == map.end())
            return 0;
        Histogram& hist = i2->second;
        if (!hist.erase(point))
           return 0;
        --size_;
        if (hist.empty()) { //Prune empty maps
            map.erase(e2);
            if (map.empty())
                storage_.erase(e1);
        }
        return 1;
    }

    size_t RemoveAll(EdgeId e1, EdgeId e2) {
        auto i1 = storage_.find(e1);
        if (i1 == storage_.end())
            return 0;
        auto& map = i1->second;
        auto i2 = map.find(e2);
        if (i2 == map.end())
            return 0;
        Histogram& hist = i2->second;
        size_t size_decrease = hist.size();
        map.erase(i2);
        size_ -= size_decrease;
        if (map.empty()) //Prune empty maps
            storage_.erase(i1);
        return size_decrease;
    }

public:

    /**
     * @brief Removes all neighbourhood of an edge (all edges referring to it, and their histograms)
     * @warning Currently doesn't check the conjugate info (should it?), so it may actually
     *          skip some data.
     * @return The number of deleted entries
     */
    size_t Remove(EdgeId edge) {
        InnerMap &inner_map = storage_[edge];
        for (auto iter = inner_map.begin(); iter != inner_map.end(); ++iter) {
            EdgeId e2 = iter->first;
            if (edge != e2) {
                this->Remove(e2, edge);
            }
        }
        size_t size_of_removed = inner_map.size();
        storage_.erase(edge);
        size_ -= size_of_removed;
        return size_of_removed;
    }

    // --Accessing--

    /**
     * @brief Underlying raw implementation data (for custom iterator helpers).
     */
    ImplIterator data_begin() const {
        return storage_.begin();
    }

    /**
     * @brief Underlying raw implementation data (for custom iterator helpers).
     */
    ImplIterator data_end() const {
        return storage_.end();
    }

    adt::iterator_range<ImplIterator> data() const {
        return adt::make_range(data_begin(), data_end());
    }

    template<bool half = false>
    EdgeProxy<half> GetT(EdgeId e) const {
        return EdgeProxy<half>(*this, GetImpl(e), e);
    }

    /**
     * @brief Returns a full proxy map to the neighbourhood of some edge.
     */
    EdgeProxy<> Get(EdgeId e) const {
        return GetT<>(e);
    }

    /**
     * @brief Returns a half proxy map to normalized neighboring edges
     * @detail You should use it when you don't care for conjugate info,
     *         or don't want to process them twice.
     */
    EdgeProxy<true> GetHalf(EdgeId e) const {
        return GetT<true>(e);
    }

    /**
     * @brief Operator alias of Get(id).
     */
    EdgeProxy<> operator[](EdgeId e) const {
        return Get(e);
    }

private:
    //When there is no such edge, returns a fake empty map for safety
    const InnerMap& GetImpl(EdgeId e) const {
        auto i = storage_.find(e);
        if (i != storage_.end())
            return i->second;
        return empty_map_;
    }

    //When there is no such histogram, returns a fake empty histogram for safety
    const Histogram& GetImpl(EdgeId e1, EdgeId e2) const {
        auto i = storage_.find(e1);
        if (i != storage_.end()) {
            auto j = i->second.find(e2);
            if (j != i->second.end())
                return j->second;
        }
        return HistProxy<>::empty_hist();
    }

public:

    /**
     * @brief Returns a full histogram proxy for all points between two edges.
     */
    HistProxy<> Get(EdgeId e1, EdgeId e2) const {
        return HistProxy<>(GetImpl(e1, e2));
    }

    /**
     * @brief Operator alias of Get(e1, e2).
     */
    HistProxy<> operator[](EdgePair p) const {
        return Get(p.first, p.second);
    }

    /**
     * @brief Returns a full backwards histogram proxy for all points between two edges.
     */
    HistProxy<true> GetBack(EdgeId e1, EdgeId e2) const {
        return HistProxy<true>(GetImpl(e1, e2));
    }

    /**
     * @brief Checks if an edge (or its conjugated twin) is consisted in the index.
     */
    bool contains(EdgeId edge) const {
        return storage_.count(edge) + storage_.count(graph_.conjugate(edge)) > 0;
    }

    /**
     * @brief Checks if there is a histogram for two points.
     */
    bool contains(EdgeId e1, EdgeId e2) const {
        auto i1 = storage_.find(e1);
        if (i1 != storage_.end() && i1->second.count(e2))
            return true;
        return false;
    }

    // --Miscellaneous--

    /**
     * Returns the graph the index is based on. Needed for custom iterators.
     */
    const Graph &graph() const { return graph_; }

    /**
     * @brief Inits the index with graph data. Used in clustered indexes.
     * @warning Do not call this on non-empty indexes.
     */
    void Init() {
        //VERIFY(size() == 0);
        for (auto it = graph_.ConstEdgeBegin(); !it.IsEnd(); ++it)
            storage_[*it][*it].insert(Point());
    }

    /**
     * @brief Clears the whole index. Used in merging.
     */
    void Clear() {
        storage_.clear();
        size_ = 0;
    }

    /**
     * @brief Returns the physical index size (total count of all edge pairs)
     */
    size_t size() const { return size_; }

private:
    size_t size_;
    const Graph& graph_;
    StorageMap storage_;
    InnerMap empty_map_; //null object
};

//Aliases for common graphs
template<typename K, typename V>
using safe_btree_map = btree::safe_btree_map<K, V>; //Two-parameters wrapper
template<typename Graph>
using PairedInfoIndexT = PairedIndex<Graph, HistogramWithWeight, safe_btree_map>;

template<typename K, typename V>
using sparse_hash_map = google::sparse_hash_map<K, V>; //Two-parameters wrapper
template<typename Graph>
using UnclusteredPairedInfoIndexT = PairedIndex<Graph, RawHistogram, sparse_hash_map>;

/**
 * @brief A collection of paired indexes which can be manipulated as one.
 *        Used as a convenient wrapper in parallel index processing.
 */
template<class Index>
class PairedIndices {
    typedef std::vector<Index> Storage;
    Storage data_;

public:
    PairedIndices() {}

    PairedIndices(const typename Index::Graph& graph, size_t lib_num) {
        data_.reserve(lib_num);
        for (size_t i = 0; i < lib_num; ++i)
            data_.emplace_back(graph);
    }

    /**
     * @brief Initializes all indexes with zero points.
     */
    void Init() { for (auto& it : data_) it.Init(); }

    /**
     * @brief Clears all indexes.
     */
    void Clear() { for (auto& it : data_) it.Clear(); }

    Index& operator[](size_t i) { return data_[i]; }

    const Index& operator[](size_t i) const { return data_[i]; }

    size_t size() const { return data_.size(); }

    typename Storage::iterator begin() { return data_.begin(); }
    typename Storage::iterator end() { return data_.end(); }

    typename Storage::const_iterator begin() const { return data_.begin(); }
    typename Storage::const_iterator end() const { return data_.end(); }
};

template<class Graph>
using PairedInfoIndicesT = PairedIndices<PairedInfoIndexT<Graph>>;

template<class Graph>
using UnclusteredPairedInfoIndicesT = PairedIndices<UnclusteredPairedInfoIndexT<Graph>>;

template<typename K, typename V>
using unordered_map = std::unordered_map<K, V>; //Two-parameters wrapper
template<class Graph>
using PairedInfoBuffer = PairedIndex<Graph, RawHistogram, unordered_map>;

template<class Graph>
using PairedInfoBuffersT = PairedIndices<PairedInfoBuffer<Graph>>;

/*
//Debug
template<typename T>
std::ostream& operator<<(std::ostream& str, const PairedInfoBuffer<T>& pi) {
    str << "--- PI of size " << pi.size() << "---\n";

    for (auto i = pi.data_begin(); i != pi.data_end(); ++i) {
        auto e1 = i->first;
        str << e1 << " has: \n";

        for (auto j = i->second.begin(); j != i->second.end(); ++j) {
            str << "- " << j->first << ": ";
            for (auto p : j->second)
                str << p << ", ";
            str << std::endl;
        }
    }

    str << "-------\n";
    return str;
}

//Debug
template<typename T>
std::ostream& operator<<(std::ostream& str, const PairedInfoIndexT<T>& pi) {
    str << "--- PI of size " << pi.size() << "---\n";

    for (auto i = pi.data_begin(); i != pi.data_end(); ++i) {
        auto e1 = i->first;
        str << e1 << " has: \n";

        for (auto j = i->second.begin(); j != i->second.end(); ++j) {
            str << "- " << j->first << ": ";
            for (auto p : j->second)
                str << p << ", ";
            str << std::endl;
        }
    }

    str << "-------\n";
    return str;
}
*/

}

}
