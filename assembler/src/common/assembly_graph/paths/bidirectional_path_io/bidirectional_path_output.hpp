//***************************************************************************
//* Copyright (c) 2015 Saint Petersburg State University
//* Copyright (c) 2011-2014 Saint Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#pragma once

#include "io_support.hpp"

namespace path_extend {

class ContigWriter {
    const Graph& g_;
    map<EdgeId, ExtendedContigId> ids_;
    shared_ptr<ContigNameGenerator> name_generator_;

    string ToFASTGPathFormat(const BidirectionalPath &path) const;
    void WriteScaffolds(const vector<ScaffoldInfo> &scaffold_storage, const string &fn) const;
    void WritePathsFastg(const vector<ScaffoldInfo> &scaffold_storage, const string &fn) const;

public:
    ContigWriter(const Graph& g,
                 const ConnectedComponentCounter &c_counter,
                 shared_ptr<ContigNameGenerator> name_generator) :
            g_(g),
            ids_(MakeEdgeIdMap(g_, c_counter, "NODE")),
            name_generator_(name_generator) {
    }

    void OutputPaths(const PathContainer &paths,
                               const string &filename_base,
                               bool write_fastg = true) const;

private:
    DECL_LOGGER("PathExtendIO")
};

}
