/**
 * @file    reader.hpp
 * @author  Mariya Fomkina
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
 * Reader is the base class that gets SingleReads or PairedReads
 * from one or two input files respectively.
 * Reader<SingleRead> is the very base class that reads from one file
 * through Parser object.
 * Reader<PairedRead> is the class that reads data from two input
 * files and gets PairedReads using this data and distance information.
 */

#ifndef COMMON_IO_READER_HPP_
#define COMMON_IO_READER_HPP_

#include "io/ireader.hpp"
#include "io/single_read.hpp"
#include "io/paired_read.hpp"
#include "io/parser.hpp"

namespace io {

/*
 * This class only represents Reader. All the functionality
 * is implemented in specializations. Thus, it's impossible to use
 * Reader<int> or Reader<std::string>. The only possible variants are
 * Reader<SingleRead> and Reader<PairedRead>.
 */
template<typename ReadType>
class Reader : public IReader<ReadType> {
};

template<>
class Reader<SingleRead> : public IReader<SingleRead> {
 public:
  /*
   * Default constructor.
   * 
   * @param filename The name of the file to be opened.
   * @param distance Doesn't have any sense here, but necessary for
   * wrappers.
   * @param offset The offset of the read quality.
   */
  explicit Reader(const SingleRead::FilenameType& filename,
                  OffsetType offset_type = PhredOffset)
      : filename_(filename), offset_type_(offset_type), parser_(NULL) {
    parser_ = SelectParser(filename_, offset_type_);
  }

  /* 
   * Default destructor.
   */
  /* virtual */ ~Reader() {
    close();
    delete parser_;
  }

  /* 
   * Check whether the stream is opened.
   *
   * @return true of the stream is opened and false otherwise.
   */
  /* virtual */ bool is_open() {
    if (parser_ != NULL) {
      return parser_->is_open();
    } else {
      return false;
    }
  }

  /* 
   * Check whether we've reached the end of stream.
   *
   * @return true if the end of stream is reached and false
   * otherwise.
   */
  /* virtual */ bool eof() {
    if (parser_ != NULL) {
      return parser_->eof();
    } else {
      return true;
    }
  }

  /*
   * Read SingleRead from stream.
   *
   * @param singleread The SingleRead that will store read data.
   *
   * @return Reference to this stream.
   */
  /* virtual */ Reader& operator>>(SingleRead& singleread) {
    if (parser_ != NULL) {
      (*parser_) >> singleread;
    }
    return *this;
  }

  /*
   * Close the stream.
   */
  /* virtual */ void close() {
    if (parser_ != NULL) {
      parser_->close();
    }
  }

  /* 
   * Close the stream and open it again.
   */
  /* virtual */ void reset() {
    if (parser_ != NULL) {
      parser_->reset();
    }
  }

 private:
  /* 
   * @variable The name of the file which stream reads from.
   */
  SingleRead::FilenameType filename_;
  /*
   * @variable Quality offset type.
   */
  OffsetType offset_type_;
  /*
   * @variable Internal stream that reads from file.
   */ 
  Parser* parser_;

  /*
   * Hidden copy constructor.
   */
  explicit Reader(const Reader<SingleRead>& reader);
  /*
   * Hidden assign operator.
   */
  void operator=(const Reader<SingleRead>& reader);
};

template<>
class Reader<PairedRead> : public IReader<PairedRead> {
 public:
  /*
   * Default constructor.
   * 
   * @param filename The pair that containes the names of two files to
   * be opened.
   * @param distance Distance between parts of PairedReads.
   * @param offset The offset of the read quality.
   */
  explicit Reader(const PairedRead::FilenameType& filename,
         size_t insert_size, bool change_order = false,
         OffsetType offset_type = PhredOffset)
      : filename_(filename), insert_size_(insert_size),
        change_order_(change_order),
        offset_type_(offset_type),
        first_(new Reader<SingleRead>(filename_.first, offset_type_)),
        second_(new Reader<SingleRead>(filename_.second, offset_type_)) {}

  /* 
   * Default destructor.
   */
  /* virtual */ ~Reader() {
    close();
    delete first_;
    delete second_;
  }

  /* 
   * Check whether the stream is opened.
   *
   * @return true of the stream is opened and false otherwise.
   */
  /* virtual */ bool is_open() {
    return first_->is_open() && second_->is_open();
  }

  /* 
   * Check whether we've reached the end of stream.
   *
   * @return true if the end of stream is reached and false
   * otherwise.
   */
  /* virtual */ bool eof() {
    return first_->eof() || second_->eof();
  }

  /*
   * Read PairedRead from stream.
   *
   * @param pairedread The PairedRead that will store read data.
   *
   * @return Reference to this stream.
   */
  /* virtual */ Reader& operator>>(PairedRead& pairedread) {
    SingleRead sr1, sr2;
    (*first_) >> sr1;
    (*second_) >> sr2;

    pairedread = PairedRead(sr1, !sr2, insert_size_);
    return *this;
  }

  /*
   * Close the stream.
   */
  /* virtual */ void close() {
    first_->close();
    second_->close();
  }

  /* 
   * Close the stream and open it again.
   */
  /* virtual */ void reset() {
    first_->reset();
    second_->reset();
  }

 private:
  /* 
   * @variable The names of the files which stream reads from.
   */
  PairedRead::FilenameType filename_;

  size_t insert_size_;

  bool change_order_;

  /*
   * @variable Quality offset type.
   */
  OffsetType offset_type_;
  /*
   * @variable The first stream (reads from first file).
   */
  Reader<SingleRead>* first_;
  /*
   * @variable The second stream (reads from second file).
   */
  Reader<SingleRead>* second_;

  /*
   * Hidden copy constructor.
   */
  explicit Reader(const Reader<PairedRead>& reader);
  /*
   * Hidden assign operator.
   */
  void operator=(const Reader<PairedRead>& reader);
};

}

#endif /* COMMON_IO_READER_HPP_ */
