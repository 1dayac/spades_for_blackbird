/*
 * read.hpp
 *
 *  Created on: 29.03.2011
 *      Author: vyahhi
 */

#ifndef READ_HPP_
#define READ_HPP_

#include <string>
#include <iostream>
#include "verify.hpp"
#include "sequence/quality.hpp"
#include "sequence/sequence.hpp"
#include "sequence/nucl.hpp"
#include "sequence/sequence_tools.hpp"
#include "simple_tools.hpp"


class Read {
public:
  static const int PHRED_OFFSET = 33;

  bool isValid() const {
    return valid_;
  }

  Sequence getSequence() const {
    VERIFY(valid_);
    return Sequence(seq_);
  }

  Sequence getSubSequence(size_t start, size_t length) const __attribute__ ((deprecated)) {
    VERIFY(length > 0 && start >= 0 && start + length <= seq_.size());
    return Sequence(seq_.substr(start, length));
  }

  Quality getQuality() const {
    VERIFY(valid_);
    return Quality(qual_);
  }

  const std::string& getSequenceString() const {
    return seq_;
  }

  const std::string& getQualityString() const {
    return qual_;
  }

  std::string getPhredQualityString(int offset = PHRED_OFFSET) const {
    std::string res = qual_;
    for (size_t i = 0; i < res.size(); ++i) {
      res[i] += offset;
    }
    return res;
  }

  const std::string& getName() const {
    return name_;
  }

  size_t size() const {
    return seq_.size();
  }

  char operator[](size_t i) const {
    VERIFY(is_nucl(seq_[i]));
    return dignucl(seq_[i]);
  }

  /**
    * trim read
    * @param ltrim first good base
    * @param rtrim last good base
    * @return whether there is anything left
    */
  bool trimLeftRight(int ltrim, int rtrim) {
    if (ltrim >= (int)seq_.size() || rtrim < 0 || rtrim < ltrim ) {
      seq_ = ""; qual_ = ""; valid_ = false; return 0;
    }
    if (ltrim > 0) {
      ltrim_ += ltrim;
      seq_.erase(0, ltrim);
      qual_.erase(0, ltrim);      
    }
    if (rtrim < (int)seq_.size()-ltrim-1) {
      rtrim_ -= ((int)seq_.size()-(rtrim-ltrim+1));
      seq_.erase(rtrim-ltrim+1, string::npos);
      qual_.erase(rtrim-ltrim+1, string::npos);
    }
    valid_ = updateValid();
    return true;
  }

  size_t trimNsAndBadQuality(int threshold) {
    int start = 0;
    for (; start < (int)seq_.size(); ++start) {
      if (seq_[start] != 'N' && (int)qual_[start] > threshold) break;
    }
    int end = 0;
    for (end = (int)seq_.size()-1; end > -1; --end) {
      if (seq_[end] != 'N' && (int)qual_[end] > threshold) break;
    }
    if (!trimLeftRight(start, end)) return 0;
    else return seq_.size();
  }

  /**
   * @param k k as in k-mer
   * @param start start point
   * @return the first starting point of a valid k-mer >=start; return -1 if no such place exists
   */
  int firstValidKmer(size_t start, size_t k) const __attribute__ ((deprecated)) {
    size_t curHypothesis = start;
    size_t i = start;
    for (; i < seq_.size(); ++i) {
      if (i >= k + curHypothesis)
        return curHypothesis;
      if (!is_nucl(seq_[i])) {
        curHypothesis = i + 1;
      }
    }
    if (i >= k + curHypothesis) {
      return curHypothesis;
    }
    return -1;
  }

  void setSequence(const char* s) {
    seq_ = s;
    valid_ = updateValid();
  }

  Read() :
    valid_(false) {
    ;
  }

  Read(const std::string &name, const std::string &seq, const std::string &qual) :
    name_(name), seq_(seq), qual_(qual) {  // for test only!
    valid_ = updateValid();
  }

  const int ltrim() { return ltrim_; }
  const int rtrim() { return rtrim_; }
  const int initial_size() { return initial_size_; }

private:
  std::string name_;
  std::string seq_;
  std::string qual_;
  bool valid_;
  int ltrim_;
  int rtrim_;
  int initial_size_;
  friend class ireadstream;
  friend uint32_t TrimBadQuality(Read*, int);
  void setName(const char* s) {
    name_ = s;
  }
  void setQuality(const char* s, int offset = PHRED_OFFSET) {
    qual_ = s;
    for (size_t i = 0; i < qual_.size(); ++i) {
      qual_[i] -= offset;
    }
  }
  const bool updateValid() const {
    if (seq_.size() == 0) {
      return false;
    }
    for (size_t i = 0; i < seq_.size(); ++i) {
      if (!is_nucl(seq_[i])) {
	return false;
      }
    }
    return true;
  }

public:
  Read operator!() const {
    std::string newName;
    if (name_ == "" || name_[0] != '!') {
      newName = '!' + name_;
    } else {
      newName = name_.substr(1, name_.length());
    }
    return Read(newName, ReverseComplement(seq_), Reverse(qual_));
  }
};

// todo: put this to *.cpp
//ostream& operator<<(ostream& os, const Read& read) {
//	return os << read.getSequenceString();
//}

#endif /* READ_HPP_ */
