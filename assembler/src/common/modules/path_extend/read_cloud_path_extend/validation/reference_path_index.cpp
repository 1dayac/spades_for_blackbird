#include <common/assembly_graph/core/graph.hpp>
#include <common/assembly_graph/graph_support/scaff_supplementary.hpp>
#include "reference_path_index.hpp"

namespace path_extend {
namespace validation {
ReferencePathIndex::EdgeInfo path_extend::validation::ReferencePathIndex::at(const debruijn_graph::EdgeId &edge) const {
    return edge_to_info_.at(edge);
}
void ReferencePathIndex::Insert(EdgeId edge, size_t path, size_t pos, size_t rev_pos) {
    EdgeInfo info(pos, rev_pos, path);
    edge_to_info_.insert({edge, info});
}
bool ReferencePathIndex::Contains(const EdgeId &edge) const {
    return edge_to_info_.find(edge) != edge_to_info_.end();
}
}
}
