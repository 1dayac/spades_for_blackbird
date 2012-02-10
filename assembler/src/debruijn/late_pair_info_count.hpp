#pragma once

#include "standard.hpp"
#include "omni/paired_info.hpp"
#include "simplification.hpp"
#include "graph_construction.hpp"
#include "omni/insert_size_refiner.hpp"

namespace debruijn_graph {

void late_pair_info_count(conj_graph_pack& gp,
		paired_info_index& paired_index) {
	exec_simplification(gp);

	INFO("STAGE == Late Pair Info Count");

	if (cfg::get().paired_mode) {
		INFO("Estimating dataset paired params");
		const size_t edge_length_threshold = 500;
		refine_insert_size(
				make_pair(input_file(cfg::get().ds.first),
						input_file(cfg::get().ds.second)), gp, edge_length_threshold);

		INFO("STAGE == Counting Late Pair Info");
		io::PairedEasyReader stream(
				make_pair(input_file(cfg::get().ds.first),
						input_file(cfg::get().ds.second)),
				cfg::get().is_infinity);

		if (cfg::get().advanced_estimator_mode)
			FillPairedIndexWithProductMetric<K>(gp.g, gp.index, gp.kmer_mapper,
					paired_index, stream);
		else
			FillPairedIndexWithReadCountMetric<K>(gp.g, gp.int_ids, gp.index,
					gp.kmer_mapper, paired_index, stream);
	}
}

void load_late_pair_info_count(conj_graph_pack& gp,
		paired_info_index& paired_index, files_t* used_files) {
	fs::path p = fs::path(cfg::get().load_from) / "late_pair_info_counted";
	used_files->push_back(p);

	ScanWithPairedIndex(p.string(), gp, paired_index);
	load_estimated_params(p.string());
}

void save_late_pair_info_count(conj_graph_pack& gp,
		paired_info_index& paired_index) {
	fs::path p = fs::path(cfg::get().output_saves) / "late_pair_info_counted";
	PrintWithPairedIndex(p.string(), gp, paired_index);
	write_estimated_params(p.string());
}

void exec_late_pair_info_count(conj_graph_pack& gp,
		paired_info_index& paired_index) {
	if (cfg::get().entry_point <= ws_late_pair_info_count) {
		late_pair_info_count(gp, paired_index);
		save_late_pair_info_count(gp, paired_index);
	} else {
		INFO("Loading Late Pair Info Count");
		files_t used_files;
		load_late_pair_info_count(gp, paired_index, &used_files);
		copy_files_by_prefix(used_files, cfg::get().output_saves);
	}
}

}

