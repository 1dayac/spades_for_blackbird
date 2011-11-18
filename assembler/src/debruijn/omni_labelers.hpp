/*
 * labeler.hpp
 *
 *  Created on: 8 Sep 2011
 *      Author: valery
 */

#pragma once

#include "omni/total_labeler.hpp"
#include "omni/graph_labeler.hpp"
#include "new_debruijn.hpp"

namespace debruijn_graph
{
typedef omnigraph::TotalLabelerGraphStruct  <ConjugateDeBruijnGraph>	total_labeler_graph_struct;
typedef omnigraph::TotalLabeler             <ConjugateDeBruijnGraph>    total_labeler;
typedef omnigraph::GraphLabeler             <ConjugateDeBruijnGraph>    graph_labeler;

} // debruijn_graph
