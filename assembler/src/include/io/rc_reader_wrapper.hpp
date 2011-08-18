/**
 * @file    rc_reader_wrapper.hpp
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
 * RCReaderWrapper is the class-wrapper that gets reads and reverse
 * complimentary reads from given reader (one by one).
 */

#ifndef COMMON_IO_RCREADERWRAPPER_HPP_
#define COMMON_IO_RCREADERWRAPPER_HPP_

#include "io/ireader.hpp"

namespace io {

template<typename ReadType>
class RCReaderWrapper : public IReader<ReadType> {
 public:
  /*
   * Default constructor.
   *
   * @param reader Pointer to any other reader (ancestor of IReader).
   */
  explicit RCReaderWrapper(IReader<ReadType>* reader)
      : reader_(reader), rc_read_(), was_rc_(true) {
  }

  /* 
   * Default destructor.
   */
  /* virtual */ ~RCReaderWrapper() {
    close();
  }

  /* 
   * Check whether the stream is opened.
   *
   * @return true of the stream is opened and false otherwise.
   */
  /* virtual */ bool is_open() {
    return reader_->is_open();
  }

  /* 
   * Check whether we've reached the end of stream.
   *
   * @return true if the end of stream is reached and false
   * otherwise.
   */
  /* virtual */ bool eof() {
    return (was_rc_) && (reader_->eof());
  }

  /*
   * Read SingleRead or PairedRead from stream (according to ReadType).
   *
   * @param read The SingleRead or PairedRead that will store read
   * data.
   *
   * @return Reference to this stream.
   */
  /* virtual */ RCReaderWrapper& operator>>(ReadType& read) {
    if (was_rc_) {
      (*reader_) >> read;
      rc_read_ = read;
    } else {
      read = !rc_read_;
    }
    was_rc_ = !was_rc_;
    return (*this);
  }

  /*
   * Close the stream.
   */
  /* virtual */ void close() {
    reader_->close();
  }

  /* 
   * Close the stream and open it again.
   */
  /* virtual */ void reset() {
    was_rc_ = true;
    reader_->reset();
  }

 private:
  /*
   * @variable Internal stream readers.
   */
  IReader<ReadType>* reader_;
  /*
   * @variable Last read got from the stream.
   */ 
  ReadType rc_read_;
  /*
   * @variable Flag that shows where reverse complimentary copy of the
   * last read read was already outputted.
   */
  bool was_rc_;

  /*
   * Hidden copy constructor.
   */
  explicit RCReaderWrapper(const RCReaderWrapper<ReadType>& reader);
  /*
   * Hidden assign operator.
   */
  void operator=(const RCReaderWrapper<ReadType>& reader);
};

}

#endif /* COMMON_IO_RCREADERWRAPPER_HPP_ */
