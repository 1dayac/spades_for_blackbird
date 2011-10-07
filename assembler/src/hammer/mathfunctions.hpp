/*
 * mathfunctions.hpp
 *
 *  Created on: 11.05.2011
 *      Author: snikolenko
 */

#ifndef MATHFUNCTIONS_H
#define MATHFUNCTIONS_H

#include <cmath>
#include "hammer_tools.hpp"
#include "position_kmer.hpp"


long double Factorial(int n) {
  if (n == 0) {
    return 1;
  }
  static unordered_map<int, long double> ans;
  if (ans.count(n) == 0) {
    ans[n] = Factorial(n - 1) * n;
  }
  return ans[n];
}

long double CNK(int n, int k) {
  return Factorial(n) / (Factorial(k) * Factorial(n - k));
}

long double Bernoulli(int k, int n, long double p) {
  return pow(p, k) * pow(1 - p, n - k) * CNK(n, k);
}


/**
  * @return logarithm of {n choose k}
  */
inline double logChooseNK(int n, int k) {
	double res = 0;
	for (int i=1; i <= k; ++i) {
		res += log(n-k+i) - log(i);
	}
	return res;
}

/**
  * @return Beta(x,y)
  */
inline double lBeta(int x, int y) {
	return (lgamma(x) + lgamma(y) - lgamma(x+y));
}

/**
  * @return log({a_1+...+a_n \choose a_1, ..., a_n})
  */
inline double lMultinomial(const vector<int> & x, const vector<KMerCount*> & k_) {
	double res = 0.0, sum = 0.0;
	for (size_t i=0; i<x.size(); ++i) {
		res += lgamma(k_[x[i]]->second.count+1);
		sum += k_[x[i]]->second.count;
	}
	return (lgamma(sum+1) - res);
}

/**
  * @return log({a_1+...+a_n \choose a_1, ..., a_n})
  */
inline double lMultinomial(const vector<KMerCount*> & x) {
	double res = 0.0, sum = 0.0;
	for (size_t i=0; i<x.size(); ++i) {
		res += lgamma(x[i]->second.count+1);
		sum += x[i]->second.count;
	}
	return (lgamma(sum+1) - res);
}

/**
  * @return log({a_1+...+a_n \choose a_1, ..., a_n})
  */
inline double lMultinomial(const vector<StringCount> & x) {
	double res = 0.0, sum = 0.0;
	for (size_t i=0; i<x.size(); ++i) {
		res += lgamma(x[i].second+1);
		sum += x[i].second;
	}
	return (lgamma(sum+1) - res);
}

/**
  * @return log({a_1+...+a_n \choose a_1, ..., a_n}) for reads corresponding to the mask
  */
inline double lMultinomialWithMask(const vector<int> & x, const vector<KMerCount*> & k_, const vector<int> & mask, int maskval) {
	assert(x.size() == mask.size());
	double res = 0.0, sum = 0.0;
	for (size_t i=0; i<x.size(); ++i) {
		if (mask[i] != maskval) continue;
		res += lgamma(k_[x[i]]->second.count+1);
		sum += k_[x[i]]->second.count;
	}
	return (lgamma(sum+1) - res);
}

/**
  * @return log(Beta(a_1+1, ..., a_n+1))
  */
inline double lBetaPlusOne(const vector<int> & x) {
	double res = 0.0, sum = 0.0;
	for (size_t i=0; i<x.size(); ++i) {
		res += lgamma(x[i]+1);
		sum += x[i]+1;
	}
	return (res - lgamma(sum));
}

#endif 
