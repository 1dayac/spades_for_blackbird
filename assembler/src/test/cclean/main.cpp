#include <iostream>
#include "utils.cpp"
#include "test_generator.hpp"

int main()
{
//    cclean_test::GenerateDataSet("/home/undead/cclean_data/ecoli_mda_lane1.fastq", "/home/undead/cclean_data/dataset_500.fastq");
    cclean_test::AreFastqFilesDifferent("/home/undead/cclean_data/output_bruteforce.fastq",
                                       "/home/undead/cclean_data/output_simple.fastq");
  return EXIT_SUCCESS;
}
