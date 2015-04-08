//***************************************************************************
//* Copyright (c) 2011-2014 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

#pragma once

#include "../environment.hpp"
#include "../command.hpp"
#include "../errors.hpp"
#include "../argument_list.hpp"
#include "../../include/omni/visualization/visualization.hpp"
#include "sequence_mapper.hpp"
#include "io/single_read.hpp"

namespace online_visualization {

class DrawingCommand : public LocalCommand<DebruijnEnvironment> {
protected:
    void DrawPicture(DebruijnEnvironment& curr_env, VertexId vertex, string label = "") const {
        make_dir(curr_env.folder_);


        stringstream namestream;
        namestream << curr_env.folder_ << "/" << curr_env.GetFormattedPictureCounter() << "_" << curr_env.file_name_base_ << "_" << label << "_" << ".dot";
        string file_name = namestream.str();
        //stringstream linkstream;
        //linkstream  << curr_env.folder_ << "/" << curr_env.file_name_base_ << "_latest.dot";
        //EdgePosGraphLabeler<Graph> labeler(curr_env.graph(), gp_.edge_pos);
        omnigraph::GraphComponent<Graph> component = VertexNeighborhood(curr_env.graph(), vertex, curr_env.max_vertices_, curr_env.edge_length_bound_);
        omnigraph::visualization::WriteComponent<Graph>(component, file_name, curr_env.coloring_, curr_env.labeler());
        //WriteComponents <Graph> (curr_env.graph(), splitter, linkstream.str(), *DefaultColorer(curr_env.graph(), curr_env.coloring_), curr_env.labeler());
        LOG("The picture is written to " << file_name);

        curr_env.picture_counter_++;
    }

    void DrawPicturesAlongPath(DebruijnEnvironment& curr_env, const vector<EdgeId>& path, string label = "") const {
        make_dir(curr_env.folder_);
        stringstream namestream;
        namestream << curr_env.folder_ << "/" << curr_env.GetFormattedPictureCounter() << "_" << curr_env.file_name_base_ << "/";
        make_dir(namestream.str());
        namestream << label;
        make_dir(namestream.str());
        omnigraph::visualization::WriteComponentsAlongPath<Graph>(curr_env.graph(), path, namestream.str(), curr_env.coloring_, curr_env.labeler());
        LOG("The pictures is written to " << namestream.str());

        curr_env.picture_counter_++;
    }

    void DrawPicturesAlongSequence(DebruijnEnvironment& curr_env, const Sequence& s, string label = "") const {
        DrawPicturesAlongPath(curr_env, curr_env.mapper().MapSequence(s).simple_path(), label);
    }

    void DrawPicturesAlongContig(DebruijnEnvironment& curr_env, io::SingleRead contig) const {
        Sequence seq = contig.sequence();
        string label = contig.name();
        DrawPicturesAlongSequence(curr_env, seq, label);
        LOG("Contig " << contig.name() << " has been drawn");
    }

    void DrawConnectedComponents (DebruijnEnvironment& curr_env,  int min_size, int max_size, string label = "") const {
        make_dir(curr_env.folder_);
        stringstream namestream;
        namestream << curr_env.folder_ << "/" << curr_env.GetFormattedPictureCounter() << "_" << curr_env.file_name_base_ << "/";
        make_dir(namestream.str());
        namestream << label;
        make_dir(namestream.str());
        omnigraph::visualization::WriteSizeLimitedComponents<Graph>(curr_env.graph(), namestream.str(), omnigraph::ConnectedSplitter<Graph>(curr_env.graph()), curr_env.coloring_, curr_env.labeler(), min_size, max_size, 10000000);
        cout << "The pictures is written to " << namestream.str() << endl;
        curr_env.picture_counter_++;
    }

    //TODO: copy zgrviewer
    int ShowPicture(DebruijnEnvironment& curr_env, VertexId vertex, string label = "") const {
        DrawPicture(curr_env, vertex, label);
        stringstream command_line_string;
        command_line_string << "gnome-open " << curr_env.folder_ << "/" << curr_env.file_name_base_
                            << "_" << label << "_" << curr_env.GetFormattedPictureCounter()
                            << "_*_.dot & > /dev/null < /dev/null";
        int result = system(command_line_string.str().c_str());

        return result;
    }

    void DrawVertex(DebruijnEnvironment& curr_env, size_t vertex_id, string label = "") const {
        DrawPicture(curr_env, curr_env.finder().ReturnVertexId(vertex_id), label);
    }


public:
    DrawingCommand(string command_type) : LocalCommand<DebruijnEnvironment>(command_type)
    {
    }

    virtual ~DrawingCommand()
    {
    }
};
}
