#ifndef TEST_PAIREDREADTEST_HPP_
#define TEST_PAIREDREADTEST_HPP_

#include "cute/cute.h"
#include "common/io/paired_read.hpp"

void TestPairedRead() {
  SingleRead sr1("Read1", "ATGCATGC", "aabbaabb");
  SingleRead sr2("Read2", "AATTGGCC", "aabbaabb");
  PairedRead pr1(sr1, sr2, 100);
  ASSERT_EQUAL(true, pr1.IsValid());
  ASSERT_EQUAL(sr1, pr1[0]);
  ASSERT_EQUAL(sr2, pr1[1]);
  PairedRead pr2(!sr2, !sr1, 100);
  ASSERT_EQUAL(pr2, !pr1);
}

cute::suite PairedReadSuite() {
  cute::suite s;
  s.push_back(CUTE(TestPairedRead));
  return s;
}

#endif /* TEST_PAIREDREADTEST_HPP_ */
