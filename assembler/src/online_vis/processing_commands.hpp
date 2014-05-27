//***************************************************************************
//* Copyright (c) 2011-2014 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

#pragma once

#include "environment.hpp"
#include "command.hpp"
#include "errors.hpp"
#include "argument_list.hpp"
#include "omni/tip_clipper.hpp"
#include "genomic_quality.hpp"

namespace online_visualization {

class ClipTipsCommand : public NewLocalCommand<DebruijnEnvironment> {

public:
    string Usage() const {
        string answer;
        answer = answer + "Command `clip_tips` \n" + "Usage:\n"
                + "> clip_tips <length>\n (Y/y)" + " This command clips tips.\n"
                + " If length is not specified, "
                + "it will be counted from global settings. "
                + "If second argument Y/y is specified then genomic edges will be retained.";
        return answer;
    }

    ClipTipsCommand()
            : NewLocalCommand<DebruijnEnvironment>("clip_tips", 1) {
    }

private:
    /*virtual*/ void InnerExecute(DebruijnEnvironment& curr_env,
                 const vector<string>& args) const {
        size_t length = GetInt(args[1]);
        shared_ptr<func::Predicate<EdgeId>> condition = make_shared<AlwaysTrue<EdgeId>>();
        if (args.size() > 2 && (args[2] == "Y" || args[2] == "y")) {
            cout << "Trying to activate genome quality condition" << endl;
            if (curr_env.genome().size() == 0) {
                cout << "No reference was provided!!!" << endl;
            } else {
                cout << "Genome quality condition will be used" << endl;

                curr_env.graph_pack().ClearQuality();
                curr_env.graph_pack().FillQuality();
//                condition = make_shared<make_shared<debruijn_graph::ZeroQualityCondition<Graph, Index>>(curr_env.graph(), edge_qual);
                condition = make_shared<func::AdaptorPredicate<EdgeId>>(boost::bind(&debruijn_graph::EdgeQuality<Graph>::IsZeroQuality
                                                                                    , boost::ref(curr_env.graph_pack().edge_qual), _1));
            }
        }
        omnigraph::ClipTips(curr_env.graph(), length, condition);
    }
};
}
