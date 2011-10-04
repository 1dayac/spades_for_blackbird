/*
 * ifastqstream.hpp
 *
 *  Created on: 03.03.2011
 *      Author: vyahhi
 */

#ifndef IREADSTREAM_HPP_
#define IREADSTREAM_HPP_

#include "kseq/kseq.h"
#include <zlib.h>
#include "verify.hpp"
#include "read/read.hpp"
#include "sequence/quality.hpp"
#include "sequence/nucl.hpp"

using namespace std;

// STEP 1: declare the type of file handler and the read() function
KSEQ_INIT(gzFile, gzread)

/*
 * Read name, seq and qual strings from FASTQ data (one by one)
 */
class ireadstream {

public:
	ireadstream(const string& filename) : offset_(Read::PHRED_OFFSET) {
		filename_ = filename;
		is_open_ = open(filename);
	}

	ireadstream(const string& filename, int offset) : offset_(offset) {
		filename_ = filename;
		is_open_ = open(filename);
	}

	virtual ~ireadstream() {
		close();
	}

	bool is_open() const {
		return is_open_;
	}

	bool eof() const {
		return eof_;
	}

	static vector<Read>* readAll(string filename, int cnt = -1) {
		ireadstream irs(filename);
		VERIFY(irs.is_open());
		vector<Read>* res = new vector<Read>();
		Read r;
		while (cnt-- && irs.is_open() && !irs.eof()) {
			irs >> r;
			if (!r.isValid()) {
				cnt++;
				continue;
			}
			res->push_back(r);
		}
		irs.close();
		return res;
	}

	static void readAllNoValidation(vector<Read>* res, string filename, uint64_t * totalsize, int qvoffset = Read::PHRED_OFFSET, int trim_quality = -1, int cnt = -1) {
		ireadstream irs(filename, qvoffset);
		VERIFY(irs.is_open());
		*totalsize = 0;
		Read r;
		while (cnt-- && irs.is_open() && !irs.eof()) {
			irs >> r;
			size_t read_size = r.trimNsAndBadQuality(trim_quality);
			res->push_back(r);
			*totalsize += read_size;
		}
		irs.close();
	}
	
	ireadstream& operator>>(Read &r) {
		VERIFY(is_open());
		VERIFY(!eof());
		if (!is_open() || eof()) {
			return *this;
		}
		r.setName(seq_->name.s);
		if (seq_->qual.s) {
			r.setQuality(seq_->qual.s, offset_);
		}
		r.setSequence(seq_->seq.s);
		r.ltrim_ = 0; r.rtrim_ = (int)r.seq_.size(); r.initial_size_ = (int)r.seq_.size();
		read_ahead(); // make actual read for the next result
		return *this;
	}

	void close() {
		if (is_open()) {
			kseq_destroy(seq_); // STEP 5: destroy seq
			gzclose(fp_); // STEP 6: close the file handler
			is_open_ = false;
		}
	}

	void reset() {
		close();
		open(filename_);
	}

private:
	std::string filename_;
	gzFile fp_;
	kseq_t* seq_;
	bool is_open_;
	bool eof_;
	bool rtl_;
	int offset_;
	/*
	 * open i's file with FASTQ reads,
	 * return true if it opened file, false otherwise
	 */
	bool open(string filename) {
		fp_ = gzopen(filename.c_str(), "r"); // STEP 2: open the file handler
		if (!fp_) {
			return false;
		}
		is_open_ = true;
		seq_ = kseq_init(fp_); // STEP 3: initialize seq
		eof_ = false;
		read_ahead();
		return true;
	}

	void read_ahead() {
		VERIFY(is_open());
		VERIFY(!eof());
		if (kseq_read(seq_) < 0) {
			eof_ = true;
		}
	}
};

#endif /* IREADSTREAM_HPP_ */
