#include <omp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "hammer_tools.hpp"
#include "subkmers.hpp"

void SubKMerSorter::runSort(std::string inputFile) {
	if (type_ == SorterTypeFileBasedStraight) {
		runFileBasedSort(inputFile);
	} else {
		runMemoryBasedSort();
	}
}

void SubKMerSorter::runMemoryBasedSort() {
	int subkmer_nthreads = max ( (tau_ + 1) * ( (int)(nthreads_ / (tau_ + 1)) ), tau_+1 );
	int effective_subkmer_threads = min(subkmer_nthreads, nthreads_);

	vector< SubKMerPQ > * vskpq = &vskpq_;

	// we divide each of (tau+1) subkmer vectors into nthreads/(tau+1) subvectors
	// as a result, we have subkmer_nthreads threads for sorting
	#pragma omp parallel for shared(vskpq) num_threads( effective_subkmer_threads )
	for (int j=0; j < subkmer_nthreads; ++j) {
		// for each j, we sort subvector (j/(tau_+1)) of the vector of subkmers at offset (j%(tau+1))
		(*vskpq)[ (j % (tau_+1)) ].doSort( j / (tau_+1), sub_less[(j % (tau_+1))] );
	}

	for (int j=0; j < tau_+1; ++j) {
		vskpq_[j].initPQ();
	}
}

void SubKMerSorter::runFileBasedSort(std::string inputFile) {

	if (!Globals::skip_sorting_subvectors) {

	TIMEDLN("Splitting " << inputFile << " into subvector files.");
	vector< ofstream* > ofs(tau_+1);
	for (int j=0; j < tau_+1; ++j) {
		ofs[j] = new ofstream(fnames_[j]);
	}
	ifstream ifs(inputFile);
	char buf[16000]; // strings might run large in those files
	hint_t line_no = 0;
	while (!ifs.eof()) {
		ifs.getline(buf, 16000);
		hint_t pos;
		sscanf(buf, "%lu", &pos);
		switch(type_) {
			case SorterTypeFileBasedStraight:
				for (int j=0; j < tau_+1; ++j) {
					ofs[j]->write(Globals::blob + pos + Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) - Globals::subKMerPositions->at(j));
					(*ofs[j]) << "\t" << line_no << "\n";
				}
				break;
			default:
				TIMEDLN("Wrong sorter type -- expected file-based.");
				exit(1);
				break;
		}
		++line_no;
	}
	ifs.close();
	for (int j=0; j < tau_+1; ++j) {
		ofs[j]->close();
		delete ofs[j];
	}

	TIMEDLN("Sorting subvector files with child processes.");
	vector< pid_t > pids(tau_+1);
	for (int j=0; j < tau_+1; ++j) {
		pids[j] = vfork();
		if ( pids[j] == 0 ) {
			TIMEDLN("  [" << getpid() << "] Child process " << j << " for sorting subkmers starting.");
			execlp("sort", "sort", "-n", "-T", Globals::working_dir.data(), "-o", sorted_fnames_[j].data(), fnames_[j].data(), (char *) 0 );
			_exit(0);
		} else if ( pids[j] < 0 ) {
			TIMEDLN("Failed to fork. Exiting.");
			exit(1);
		}
	}
	for (int j = 0; j < tau_+1; ++j) {
	    int status;
	    while (-1 == waitpid(pids[j], &status, 0));
	    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
	        TIMEDLN("Process " << j << " (pid " << pids[j] << ") failed. Exiting.");
	        exit(1);
	    }
	}

	} else {
		TIMEDLN("Skipping sorting subvectors, initializing priority queues from existing files.");
	}

	for (int j=0; j < tau_+1; ++j) {
		vskpq_[j].initPQ();
	}
}

bool SubKMerSorter::getNextBlock( int i, vector<hint_t> & block ) {
	block.clear();
	if ( vskpq_[i].emptyPQ() ) return false;
	// cout << "  new block with respect to " << i << endl;
	hint_t last = vskpq_[i].peekPQ();
	while (!vskpq_[i].emptyPQ()) {
		hint_t cur = vskpq_[i].peekPQ();
		if ( sub_equal[i](last, cur) ) { //add to current reads
			block.push_back(cur);
			// cout << "     " << cur << "\n";
			vskpq_[i].popPQ();
		} else {
			return true;
		}
	}
	return (block.size() > 0);
}

SubKMerSorter::SubKMerSorter( size_t kmers_size, vector<KMerCount*> * k, int nthreads, int tau, SubKMerSorterType type ) :
		type_(type), nthreads_(nthreads), tau_(tau), kmers_size_(kmers_size), kmers_(NULL) {
	// we set the sorting functions depending on the type
	// here the sorting functions are regular sorting functions predefined in PositionKMer
	switch( type ) {
		case SorterTypeStraight:
			for (int j=0; j < tau+1; ++j) {
				sub_less.push_back(    boost::bind(PositionKMer::compareSubKMers,        _1, _2, k, tau, 
					Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
				sub_greater.push_back( boost::bind(PositionKMer::compareSubKMersGreater, _1, _2, k, tau, 
					Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
				sub_equal.push_back(   boost::bind(PositionKMer::equalSubKMers,          _1, _2, k, tau, 
					Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
			}
			break;
		case SorterTypeChequered:
			for (int j=0; j < tau+1; ++j) {
				sub_less.push_back(    boost::bind(PositionKMer::compareSubKMersCheq,        _1, _2, k, tau, j) );
				sub_greater.push_back( boost::bind(PositionKMer::compareSubKMersGreaterCheq, _1, _2, k, tau, j) );
				sub_equal.push_back(   boost::bind(PositionKMer::equalSubKMersCheq,          _1, _2, k, tau, j) );
			}
			break;
		case SorterTypeChequeredDirect:
			for (int j=0; j < tau+1; ++j) {
				sub_less.push_back(    boost::bind(PositionKMer::compareSubKMersCheqDirect,        _1, _2, tau, j) );
				sub_greater.push_back( boost::bind(PositionKMer::compareSubKMersGreaterCheqDirect, _1, _2, tau, j) );
				sub_equal.push_back(   boost::bind(PositionKMer::equalSubKMersCheqDirect,          _1, _2, tau, j) );
			}
			break;
		case SorterTypeFileBasedStraight:
			for (int j=0; j < tau+1; ++j) {
				sub_less.push_back(    boost::bind(PositionKMer::compareSubKMersDirect,        _1, _2, tau,
						Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
				sub_greater.push_back( boost::bind(PositionKMer::compareSubKMersGreaterDirect, _1, _2, tau,
						Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
				sub_equal.push_back(   boost::bind(PositionKMer::equalSubKMersDirect,          _1, _2, tau,
						Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
				fnames_.push_back( getFilename(Globals::working_dir, Globals::iteration_no, "subkmers", j) );
				sorted_fnames_.push_back( getFilename(Globals::working_dir, Globals::iteration_no, "subkmers.sorted", j) );
			}
			break;
	}

	// initialize the vectors
	initVectors();
}

SubKMerSorter::SubKMerSorter( size_t kmers_size, vector<hint_t> * k, int nthreads, int tau, SubKMerSorterType type ) :
		type_(type), nthreads_(nthreads), tau_(tau), kmers_size_(kmers_size), kmers_(k) {
	// we set the sorting functions depending on the type
	// here the sorting functions are regular sorting functions predefined in PositionKMer
	switch( type ) {
		case SorterTypeFileBasedStraight:
			for (int j=0; j < tau+1; ++j) {
				sub_less.push_back(    boost::bind(PositionKMer::compareSubKMersHInt,        _1, _2, k, tau,
						Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
				sub_greater.push_back( boost::bind(PositionKMer::compareSubKMersGreaterHInt, _1, _2, k, tau,
						Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
				sub_equal.push_back(   boost::bind(PositionKMer::equalSubKMersHInt,          _1, _2, k, tau,
						Globals::subKMerPositions->at(j), Globals::subKMerPositions->at(j+1) ) );
				fnames_.push_back( getFilename(Globals::working_dir, Globals::iteration_no, "subkmers", j) );
				sorted_fnames_.push_back( getFilename(Globals::working_dir, Globals::iteration_no, "subkmers.sorted", j) );
			}
			break;
		default:
			assert( type == SorterTypeFileBasedStraight ); // other types don't use hint_t vectors yet
	}

	// initialize the vectors
	initVectors();
}

SubKMerSorter::~SubKMerSorter() {
	for (size_t j=0; j<vskpq_.size(); ++j) {
		vskpq_[j].closePQ();
	}
	if ( v_ != NULL ) {
		for (size_t j=0; j<v_->size(); ++j) v_->at(j).clear();
		delete v_;
	}
}

SubKMerSorter::SubKMerSorter( vector< hint_t > * kmers, vector<KMerCount*> * k, int nthreads, int tau, int jj,
	SubKMerSorterType type, SubKMerSorterType parent_type ) : nthreads_(nthreads), tau_(tau), kmers_size_(kmers->size()), kmers_(kmers) {

	//cout << "    constructor nthreads=" << nthreads << " tau=" << tau << " kmerssize=" << kmers_size_ << " jj=" << jj << endl;
	// we set the sorting functions depending on the type
	// for sorting a specific block, we use sorting functions with specific exemptions depending on jj
	if( type == SorterTypeStraight) {
		assert(parent_type == SorterTypeStraight);

		vector< pair<uint32_t, uint32_t> > my_positions(tau+1);
		uint32_t left_size = Globals::subKMerPositions->at(jj);
		uint32_t right_size = K - Globals::subKMerPositions->at(jj+1);
		uint32_t total_size = left_size + right_size;
		uint32_t left_end = ( (tau + 1) * left_size ) / ( total_size );
		uint32_t increment = total_size / (tau+1);

		for (uint32_t i=0; i < left_end; ++i) my_positions[i] = make_pair( i * increment, (i+1) * increment );
		if (left_end > 0) my_positions[left_end-1].second = left_size;
		for (uint32_t i=left_end; i < (uint32_t)tau+1; ++i) my_positions[i] = make_pair(
			Globals::subKMerPositions->at(jj+1) + (i  -left_end) * increment,
			Globals::subKMerPositions->at(jj+1) + (i+1-left_end) * increment );
		if (jj < tau) my_positions[tau].second = K;

		for (int j=0; j < tau+1; ++j) {
			sub_less.push_back(    boost::bind(PositionKMer::compareSubKMers,        _1, _2, k, tau,
				my_positions[j].first, my_positions[j].second ) );
			sub_greater.push_back( boost::bind(PositionKMer::compareSubKMersGreater, _1, _2, k, tau,
				my_positions[j].first, my_positions[j].second ) );
			sub_equal.push_back(   boost::bind(PositionKMer::equalSubKMers,          _1, _2, k, tau,
				my_positions[j].first, my_positions[j].second ) );
		}
	} else if ( type == SorterTypeChequered ) {
		assert(parent_type == SorterTypeStraight); // yet to implement a chequered second level
		for (int j=0; j < tau+1; ++j) {
			sub_less.push_back(    boost::bind(PositionKMer::compareSubKMersCheq,        _1, _2, k, tau+1, j) );
			sub_greater.push_back( boost::bind(PositionKMer::compareSubKMersGreaterCheq, _1, _2, k, tau+1, j) );
			sub_equal.push_back(   boost::bind(PositionKMer::equalSubKMersCheq,          _1, _2, k, tau+1, j) );
		}
	}

	// initialize the vectors
	initVectors();
}

SubKMerSorter::SubKMerSorter( vector< hint_t > * kmers, vector<hint_t> * v, int nthreads, int tau, int jj,
	SubKMerSorterType type, SubKMerSorterType parent_type ) : nthreads_(nthreads), tau_(tau), kmers_size_(kmers->size()), kmers_(kmers) {
	assert( type == SorterTypeChequeredDirect );
	assert(parent_type == SorterTypeStraight); // yet to implement a chequered second level
	for (int j=0; j < tau+1; ++j) {
		sub_less.push_back(    boost::bind(PositionKMer::compareSubKMersCheqHInt,        _1, _2, v, tau+1, j) );
		sub_greater.push_back( boost::bind(PositionKMer::compareSubKMersGreaterCheqHInt, _1, _2, v, tau+1, j) );
		sub_equal.push_back(   boost::bind(PositionKMer::equalSubKMersCheqHInt,          _1, _2, v, tau+1, j) );
	}

	// initialize the vectors
	initVectors();
}

void SubKMerSorter::initVectors() {
	v_ = new vector< vector<hint_t> >(tau_+1);
	int nthreads_per_subkmer = max( (int)(nthreads_ / (tau_ + 1)), 1);

	for (int j=0; j<tau_+1; ++j) {
		(*v_)[j].resize( kmers_size_ );
		for (size_t m = 0; m < kmers_size_; ++m) (*v_)[j][m] = m;
		SubKMerCompType sort_greater = boost::bind(SubKMerPQElement::functionSubKMerPQElement, _1, _2, sub_greater[j]);
		if ( type_ == SorterTypeFileBasedStraight ) {
			SubKMerPQ skpq( vector<string>(1, sorted_fnames_[j]), nthreads_per_subkmer, sort_greater );
			vskpq_.push_back(skpq);
		} else {
			SubKMerPQ skpq( &((*v_)[j]), nthreads_per_subkmer, sort_greater );
			vskpq_.push_back(skpq);
		}
	}
}

SubKMerPQ::SubKMerPQ( vector<hint_t> * vec, int nthr, SubKMerCompType sort_routine ) : boundaries(nthr + 1), v(vec), nthreads(nthr), pq(sort_routine), it(nthr), it_end(nthr) {
	// find boundaries of the pieces
	size_t sub_size = (size_t)(v->size() / nthreads);
	for (int j=0; j<nthreads; ++j) {
		boundaries[j] = j * sub_size;
	}
	boundaries[nthreads] = v->size();
}

SubKMerPQ::SubKMerPQ( const vector<string> & fnames, int nthr, SubKMerCompType sort_routine ) : boundaries(nthr + 1), v(NULL), nthreads(nthr), pq(sort_routine), it(nthr), it_end(nthr) {
	for (size_t j=0; j<fnames.size(); ++j) {
		fnames_.push_back(fnames[j]);
	}
}

void SubKMerPQ::doSort(int j, const SubKMerFunction & sub_sort) {
	sort(v->begin() + boundaries[j], v->begin() + boundaries[j+1], sub_sort);
}

hint_t SubKMerPQ::getNextElementFromFile(size_t j) {
	ifs_[j]->getline(buf_, 2048);
	hint_t kmerno;
	if ( !strlen(buf_) || sscanf(buf_, "%*s\t%lu", &kmerno) < 1 ) {
		return BLOBKMER_UNDEFINED;
	}
	return kmerno;
}

void SubKMerPQ::initPQ() {
	if (v != NULL) {
		for (int j = 0; j < nthreads; ++j) {
			it[j] = v->begin() + boundaries[j];
			it_end[j] = v->begin() + boundaries[j + 1];
			pq.push(SubKMerPQElement(*(it[j]), j));
		}
	} else {
		for (size_t j = 0; j < fnames_.size(); ++j) {
			cout << "  initializing SubKMerPQ with " <<fnames_[j].data() << endl;
			ifs_.push_back(new ifstream(fnames_[j].data()));
			if (!ifs_[j]->eof()) {
				hint_t nextel = getNextElementFromFile(j);
				if (nextel != BLOBKMER_UNDEFINED) pq.push( SubKMerPQElement( nextel, j ) );
			}
		}
	}
}

void SubKMerPQ::closePQ() {
	if (v == NULL) {
		for (size_t j = 0; j < ifs_.size(); ++j) {
			ifs_[j]->close();
			delete ifs_[j];
		}
	}
}

hint_t SubKMerPQ::nextPQ() {
	hint_t res = peekPQ(); popPQ();
	return res;
}

void SubKMerPQ::popPQ() {
	SubKMerPQElement pqel = pq.top(); pq.pop();
	if (v != NULL) {
		++it[pqel.n];
		if ( it[pqel.n] != it_end[pqel.n] ) pq.push( SubKMerPQElement(*(it[pqel.n]), pqel.n) );
	} else {
		if ( !ifs_[pqel.n]->eof() ) {
			hint_t nextel = getNextElementFromFile(pqel.n);
			if (nextel != BLOBKMER_UNDEFINED) pq.push( SubKMerPQElement( nextel, pqel.n ) );
		}
	}
}

