#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <vector>

using std::cout;
using std::cerr;
using std::ifstream;
using std::string;
using std::array;
using std::vector;

typedef unsigned char byte;

template<typename T>
T min(T a, T b);

unsigned int toUInt(const vector<byte>& bytes);

template<unsigned long int S>
unsigned int toUInt(const array<byte, S>& bytes);

string toString(const vector<byte>& bytes);

template<unsigned long int S>
string toString(const array<byte, S>& bytes);

vector<byte> toVec(unsigned int val);

template<typename T>
bool allSame(const vector<T>& vec, int n);

template<typename T>
bool contains(const vector<T>& vec, T item);

void quit(string msg);

template<typename T>
T min(T a, T b)
{
	if (a > b)
		return b;
	
	return a;
}

/*
The following four functions deal with conversion from
an vector of bytes to more workable structures. Two kinds
of each function exist, one which takes a vector and one which
takes an array. Consider rewriting other code to only require
one version of each function.
*/

unsigned int toUInt(const vector<byte>& bytes)
{
	unsigned int result = 0;

	for (int x = 0; x < bytes.size(); ++x)
		result += bytes[x] << ( 8 * (bytes.size() - 1 - x) );

	return result;
}

template<unsigned long int S>
unsigned int toUInt(const array<byte, S>& bytes)
{
	unsigned int result = 0;

	for (int x = 0; x < bytes.size(); ++x)
		result += bytes[x] << ( 8 * (bytes.size() - 1 - x) );

	return result;
}

string toString(const vector<byte>& bytes)
{
	string result;

	for (int x = 0; x < bytes.size(); ++x)
		result += bytes[x];

	return result;
}

template<unsigned long int S>
string toString(const array<byte, S>& bytes)
{
	string result;

	for (int x = 0; x < bytes.size(); ++x)
		result += bytes[x];

	return result;
}

// backwards conversion from vector of bytes to unsigned int
vector<byte> toVec(unsigned int val)
{
	vector<byte> result;

	for (int x = 0; x < 4; ++x)
		result.push_back( ( val >> ( 8 * (3 - x) ) ) & 0xFF );

	return result;
}

// returns true if all elements from vec[0] to vec[n - 1] are the same
// will return true in the case where vector size is 0/1
template<typename T>
bool allSame(const vector<T>& vec, int n)
{
	if (n <= 1)
		return true;

	for (int x = 1; x < n; ++x)
		if ( vec[x] != vec[x - 1] )
			return false;

	return true;
}

// returns true if given item is present in the given vector
template<typename T>
bool contains(const vector<T>& vec, T item)
{
	for (T elem:vec)
		if (elem == item)
			return true;

	return false;
}

// for convenience (saves ~3 lines per error handle)
void quit(string msg)
{
	cerr << "error: " << msg;
	exit(1);
}

#endif