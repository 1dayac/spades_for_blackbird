#include "quake_enhanced/quake.hpp"
#include <string>
#include <fstream>
#include <iostream>
using std::string;
using std::endl;
using quake_enhanced::Quake;
void Quake::FilterTrusted(string ifile, string ofile, string badfile) {
  ifstream in(ifile.c_str());
  ofstream out(ofile.c_str());
  ofstream bad(badfile.c_str());
  string kmer;
  int count;
  float q_count;
  float freq;
  std::cerr << ofile << endl;
  while (in >> kmer >> count >> q_count >> freq) {
    if (q_count > limits_[count]) {
      out << kmer << " " << count << " " << q_count << " " << freq << endl;
    } else {
      bad << kmer << " " << count << " " << q_count << " " << freq << endl;
    }
  }
}
