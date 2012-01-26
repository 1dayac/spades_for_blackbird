/*
 * config_struct_hammer.hpp
 *
 *  Created on: Aug 15, 2011
 *      Author: snikolenko
 */

#ifndef CONFIG_STRUCT_HAMMER_HPP_
#define CONFIG_STRUCT_HAMMER_HPP_

#include "config_common.hpp"
#include "boost/optional.hpp"

#define CONFIG_FILENAME "/home/snikolenko/algorithmic-biology/assembler/src/hammer/config.inp"

// struct for debruijn project's configuration file
struct hammer_config
{
	int input_numfiles;
	bool input_paired;
	string input_file_0;
	string input_file_1;
	string input_file_2;
	string input_file_3;
	string input_file_4;
	string input_solid_kmers;
	string input_working_dir;
	int input_trim_quality;
	boost::optional<int> input_qvoffset_opt;
	int input_qvoffset;
	bool input_gzipped;
	bool input_read_solid_kmers;

	bool general_do_everything_after_first_iteration;
	bool general_reconstruct_only;
	bool general_change_n_to_a;
	bool general_gzip;
	int general_hard_memory_limit;
	int general_max_nthreads;
	int general_tau;
	int general_max_iterations;
	double general_blob_margin;
	double general_gzip_margin;

	bool count_do;
	int count_numfiles;
	int count_merge_nthreads;

	bool sort_do;

	bool subvectors_do;
	int subvectors_blocksize_quadratic_threshold;

	bool hamming_do;
	bool hamming_write_solid_kmers;
	bool hamming_write_bad_kmers;
	int hamming_class_buffer;

	bool bayes_do;
	int bayes_nthreads;
	double bayes_quality_threshold;
	double bayes_singleton_threshold;
	double bayes_nonsingleton_threshold;
	bool bayes_discard_only_singletons;
	bool bayes_debug_output;
	bool bayes_use_hamming_dist;
	bool bayes_hammer_mode;

	bool expand_do;
	int expand_max_iterations;
	int expand_nthreads;
	bool expand_write_each_iteration;
	bool expand_write_kmers_result;

	bool correct_do;
	bool correct_use_threshold;
	double correct_threshold;
	int correct_nthreads;
};


// main debruijn config load function
void load(hammer_config& cfg, boost::property_tree::ptree const& pt);

typedef config_common::config<hammer_config> cfg;

#endif

