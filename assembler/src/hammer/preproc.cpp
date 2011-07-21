/**
 * @file    preproc.cpp
 * @author  Alex Davydow
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * @section DESCRIPTION
 *
 * For each k-mer this program calculates number of occurring in
 * the reads provided. Reads file is supposed to be in fastq
 * format.
 */
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include "log4cxx/logger.h"
#include "log4cxx/basicconfigurator.h"
#include "common/read/ireadstream.hpp"
#include "common/read/read.hpp"
#include "hammer/kmer_freq_info.hpp"
#include "hammer/valid_kmer_generator.hpp"

using std::string;
using std::set;
using std::vector;
using std::unordered_map;
using std::map;
using log4cxx::LoggerPtr;
using log4cxx::Logger;
using log4cxx::BasicConfigurator;

namespace {

const uint32_t kK = 2;
typedef Seq<kK> KMer;
typedef unordered_map<KMer, KMerFreqInfo, KMer::hash> UnorderedMap;

LoggerPtr logger(Logger::getLogger("preproc"));
/**
 * @variable Every kStep k-mer will appear in the log.
 */
const int kStep = 1e5;

struct Options {
  /**
   * @variable An offset for quality in a fastq file.
   */
  uint32_t qvoffset;
  string ifile;
  string ofile;
  /**
   * @variable How many files will be used when splitting k-mers.
   */
  uint32_t file_number;
  bool q_mers;
  bool valid;
  Options()
      : qvoffset(0),
        ifile(""),
        ofile(""),
        file_number(3),
        q_mers(false),
        valid(true) {}
};

void PrintHelp() {
  printf("Usage: ./preproc qvoffset ifile.fastq ofile.[q]cst file_number [q]\n");
  printf("Where:\n");
  printf("\tqvoffset\tan offset of fastq quality data\n");
  printf("\tifile.fastq\tan input file with reads in fastq format\n");
  printf("\tofile.[q]cst\ta filename where k-mer statistics will be outputted\n");
  printf("\tfile_number\thow many files will be used when splitting k-mers\n");
  printf("\tq\t\tif you want to count q-mers instead of k-mers\n");
}

Options ParseOptions(int argc, char *argv[]) {
  Options ret;
  if (argc != 6 && argc != 5) {
    ret.valid =  false;
  } else {
    ret.qvoffset = atoi(argv[1]);
    ret.valid &= (ret.qvoffset >= 0 && ret.qvoffset <= 255);
    ret.ifile = argv[2];
    ret.ofile = argv[3];
    ret.file_number = atoi(argv[4]);
    if (argc == 6) {
      if (string(argv[5]) == "q") {
        ret.q_mers = true;
      } else {
        ret.valid = false;
      }
    }
  }
  return ret;
}

/**
 * This function reads reads from the stream and splits them into
 * k-mers. Then k-mers are written to several file almost
 * uniformly. It is guaranteed that the same k-mers are written to the
 * same files.
 * @param ifs Steam to read reads from.
 * @param ofiles Files to write the result k-mers. They are written
 * one per line.
 */
void SplitToFiles(ireadstream ifs, const vector<FILE*> &ofiles, bool q_mers) {
  uint32_t file_number = ofiles.size();
  uint64_t read_number = 0;
  while (!ifs.eof()) {
    ++read_number;
    if (read_number % kStep == 0) {
      LOG4CXX_INFO(logger, "Reading read " << read_number << ".");
    }
    Read r;
    ifs >> r;
    KMer::hash hash_function;
    for (ValidKMerGenerator<kK> gen(r); gen.HasMore(); gen.Next()) {
      FILE *cur_file = ofiles[hash_function(gen.kmer()) % file_number];     
      KMer kmer = gen.kmer();
      if (KMer::less2()(!kmer, kmer)) {
        kmer = !kmer;
      }
      fwrite(kmer.str().c_str(), 1, kK, cur_file);
      if (q_mers) {
        double correct_probability = gen.correct_probability();
        fwrite(&correct_probability, sizeof(correct_probability), 1, cur_file);
      }
    }
  }
}

/**
 * This function reads k-mer and calculates number of occurrences for
 * each of them.
 * @param ifile File with k-mer to process. One per line.
 * @param ofile Output file. For each unique k-mer there will be a
 * line with k-mer itself and number of its occurrences.
 */
template<typename KMerStatMap>
void EvalFile(FILE *ifile, FILE *ofile, bool q_mers) {
  KMerStatMap stat_map;
  char buffer[kK + 1];
  buffer[kK] = 0;
  while (fread(buffer, 1, kK, ifile) == kK) {
    KMer kmer(buffer);
    KMerFreqInfo &info = stat_map[kmer];
    if (q_mers) {
      double correct_probability;
      assert(fread(&correct_probability, sizeof(correct_probability), 1, ifile) == 1);
      info.q_count += correct_probability;
    } else {
      info.count += 1;
    }
  }
  for (typename KMerStatMap::iterator it = stat_map.begin();
       it != stat_map.end(); ++it) {
    fprintf(ofile, "%s ", it->first.str().c_str());
    if (q_mers) {
      fprintf(ofile, "%f\n", it->second.q_count);
    } else {
      fprintf(ofile, "%d\n", it->second.count);
    }
  }
}
}

int main(int argc, char *argv[]) {
  Options opts = ParseOptions(argc, argv);
  if (!opts.valid) {
    PrintHelp();
    return 1;
  }
  BasicConfigurator::configure();
  LOG4CXX_INFO(logger, "Starting preproc: evaluating " 
               << opts.ifile << ".");
  vector<FILE*> ofiles(opts.file_number);
  for (uint32_t i = 0; i < opts.file_number; ++i) {
    char filename[50];
    snprintf(filename, sizeof(filename), "%u.kmer.part", i);
    ofiles[i] = fopen(filename, "wb");
  }
  SplitToFiles(ireadstream(opts.ifile, opts.qvoffset), ofiles, opts.q_mers);
  for (uint32_t i = 0; i < opts.file_number; ++i) {
    fclose(ofiles[i]);
  }
  
  FILE *ofile = fopen(opts.ofile.c_str(), "w");
  for (uint32_t i = 0; i < opts.file_number; ++i) {
    char ifile_name[50];
    snprintf(ifile_name, sizeof(ifile_name), "%u.kmer.part", i);
    FILE *ifile = fopen(ifile_name, "rb");
    LOG4CXX_INFO(logger, "Processing " << ifile_name << ".");
    EvalFile<UnorderedMap>(ifile, ofile, opts.q_mers);
    LOG4CXX_INFO(logger, "Processed " << ifile_name << ".");
    fclose(ifile);
  }
  fclose(ofile);
  LOG4CXX_INFO(logger,
               "Preprocessing done. You can find results in " <<
               opts.ofile << ".");
  return 0;
}
