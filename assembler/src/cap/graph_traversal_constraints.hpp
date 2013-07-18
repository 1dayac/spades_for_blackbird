#pragma once

#include "coordinates_handler.hpp"
#include "coloring.hpp"

namespace cap {

template <class Graph>
class GraphTraversalConstraints {
 public:
  typedef typename Graph::EdgeId EdgeId;

  GraphTraversalConstraints() {
  }

  virtual void PushEdge(const EdgeId edge) {
    // do nothing
  }

  virtual void PopEdge() {
    // do nothing
  }

  virtual bool PathIsCorrect() const {
    return true;
  }

 private:
  DECL_LOGGER("GraphTraversalConstraints")
    ;
};

template <class Graph>
class GenomeContiguousPathsGraphTraversalConstraints
    : public GraphTraversalConstraints<Graph> {

 public:
  typedef std::vector<std::pair<unsigned char, size_t> > PosArray;

  GenomeContiguousPathsGraphTraversalConstraints(
      const CoordinatesHandler<Graph> &coordinates_handler)
      : coordinates_handler_(coordinates_handler),
        pos_array_queue_() {
  }

  virtual void PushEdge(const EdgeId edge) {
    if (pos_array_queue_.size() == 0)
      pos_array_queue_.push(coordinates_handler_.GetEndPosArray(edge));
    else
      pos_array_queue_.push(
          coordinates_handler_.FilterPosArray(pos_array_queue_.front(), edge));
  }

  virtual void PopEdge() {
    pos_array_queue_.pop();
  }

  virtual bool PathIsCorrect() const {
    return pos_array_queue_.front().size() > 0;
  }

 private:
  const CoordinatesHandler<Graph> &coordinates_handler_;

  std::queue<PosArray> pos_array_queue_;
};

}
