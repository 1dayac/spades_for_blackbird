/*
 * config_struct_hammer.cpp
 *
 *  Created on: Oct 15, 2011
 *      Author: snikolenko
 */

#include "config_struct_hammer.hpp"

void load(hammer_config& cfg, boost::property_tree::ptree const& pt)
{
	using config_common::load;
	load(cfg.general_do_everything_after_first_iteration, pt, "general_do_everything_after_first_iteration");
	load(cfg.general_reconstruct_only, pt, "general_reconstruct_only");
	load(cfg.general_change_n_to_a, pt, "general_change_n_to_a");
	load(cfg.general_gzip, pt, "general_gzip");
	load(cfg.general_hard_memory_limit, pt, "general_hard_memory_limit");
	load(cfg.general_max_nthreads, pt, "general_max_nthreads");
	load(cfg.general_tau, pt, "general_tau");
	load(cfg.general_max_iterations, pt, "general_max_iterations");
	load(cfg.general_blob_margin, pt, "general_blob_margin");

	load(cfg.count_do, pt, "count_do");
	load(cfg.count_numfiles, pt, "count_numfiles");
	load(cfg.count_merge_nthreads, pt, "count_merge_nthreads");

	load(cfg.sort_do, pt, "sort_do");

	load(cfg.subvectors_do, pt, "subvectors_do");
	load(cfg.subvectors_blocksize_quadratic_threshold, pt, "subvectors_blocksize_quadratic_threshold");

	load(cfg.hamming_do, pt, "hamming_do");
	load(cfg.hamming_write_bad_kmers, pt, "hamming_write_bad_kmers");
	load(cfg.hamming_write_solid_kmers, pt, "hamming_write_solid_kmers");
	load(cfg.hamming_class_buffer, pt, "hamming_class_buffer");

	load(cfg.bayes_do, pt, "bayes_do");
	load(cfg.bayes_nthreads, pt, "bayes_nthreads");
	load(cfg.bayes_quality_threshold, pt, "bayes_quality_threshold");
	load(cfg.bayes_singleton_threshold, pt, "bayes_singleton_threshold");
	load(cfg.bayes_nonsingleton_threshold, pt, "bayes_nonsingleton_threshold");
	load(cfg.bayes_discard_only_singletons, pt, "bayes_discard_only_singletons");
	load(cfg.bayes_debug_output, pt, "bayes_debug_output");
	load(cfg.bayes_use_hamming_dist, pt, "bayes_use_hamming_dist");
	load(cfg.bayes_hammer_mode, pt, "bayes_hammer_mode");

	load(cfg.expand_do, pt, "expand_do");
	load(cfg.expand_max_iterations, pt, "expand_max_iterations");
	load(cfg.expand_nthreads, pt, "expand_nthreads");
	load(cfg.expand_write_each_iteration, pt, "expand_write_each_iteration");
	load(cfg.expand_write_kmers_result, pt, "expand_write_kmers_result");

	load(cfg.correct_do, pt, "correct_do");
	load(cfg.correct_nthreads, pt, "correct_nthreads");
	load(cfg.correct_threshold, pt, "correct_threshold");
	load(cfg.correct_use_threshold, pt, "correct_use_threshold");

	load(cfg.input_numfiles, pt, "input_numfiles");
	if (cfg.input_numfiles > 0) load(cfg.input_file_0, pt, "input_file_0");
	if (cfg.input_numfiles > 1) load(cfg.input_file_1, pt, "input_file_1");
	if (cfg.input_numfiles > 2) load(cfg.input_file_2, pt, "input_file_2");
	if (cfg.input_numfiles > 3) load(cfg.input_file_3, pt, "input_file_3");
	if (cfg.input_numfiles > 4) load(cfg.input_file_4, pt, "input_file_4");
	load(cfg.input_paired, pt, "input_paired");
	load(cfg.input_solid_kmers, pt, "input_solid_kmers");
	load(cfg.input_working_dir, pt, "input_working_dir");
	load(cfg.input_trim_quality, pt, "input_trim_quality");
	load(cfg.input_qvoffset, pt, "input_qvoffset");
	load(cfg.input_read_solid_kmers, pt, "input_read_solid_kmers");
	load(cfg.input_solid_kmers, pt, "input_solid_kmers");
}
