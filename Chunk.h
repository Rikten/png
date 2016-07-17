#ifndef CHUNK_H
#define CHUNK_H

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include "utils.h"

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::setfill;
using std::setw;
using std::dec;
using std::hex;
using std::string;
using std::vector;

const int TEST_BIT = 5;

/*
Auxilliary class to hold the different chunks that make up
a PNG image.
*/
class Chunk
{
private:
	vector<byte> name;
	vector<byte> data;
	vector<byte> crc;
	// size of data field is implicit in data.size()

public:
	void setName(const vector<byte>& v) { name = v; }
	void setData(const vector<byte>& v) { data = v; }
	void setCrc(const vector<byte>& v) { crc = v; }

	void addNameByte(byte b) { name.push_back(b); }
	void addDataByte(byte b) { data.push_back(b); }
	void addCrcByte(byte b) { crc.push_back(b); }

	vector<byte> getName() { return name; }
	vector<byte> getData() { return data; }
	vector<byte> getCrc() { return crc; }

	vector<byte> getCrcData();

	bool isAncillary();
	bool isPrivate();
	bool isReserved();
	bool isSafeToCopy();

	void reset();

	void print();
	void write(ofstream& out);
};

// returns the stream of data for the chunk that the checksum
// is calculated over (name bytes + data bytes)
vector<byte> Chunk::getCrcData()
{
	vector<byte> result;

	for (byte elem:name)
		result.push_back(elem);

	for (byte elem:data)
		result.push_back(elem);

	return result;
}

/*  Functions to get various properties of a chunk

	From https://www.w3.org/TR/PNG/#5Chunk-naming-conventions:
	"... decoders should test the properties of an unknown chunk type 
	 by numerically testing the specified bits; testing whether a character 
	 is uppercase or lowercase is inefficient, and even incorrect if a 
	 locale-specific case definition is used."
*/

bool Chunk::isAncillary()
{
	return name[0] & (1 << TEST_BIT);
}

bool Chunk::isPrivate()
{
	return name[1] & (1 << TEST_BIT);
}

bool Chunk::isReserved()
{
	return name[2] & (1 << TEST_BIT);
}

bool Chunk::isSafeToCopy()
{
	return name[3] & (1 << TEST_BIT);
}

// currently used on a temp chunk when filling the PNG's chunk
// vector, consider rewriting that part to avoid the need for this
void Chunk::reset()
{
	name.clear();
	data.clear();
	crc.clear();
}

// print an overview of the chunk's contents
void Chunk::print()
{
	cout << "Chunk name: " << toString(name) << endl
		<< data.size() << " bytes long\n"
		<< "CRC: " << hex << setfill('0');

	for (byte elem:crc)
		cout << setw(2) << static_cast<int>(elem) << ' ';

	cout << dec << "\n\n";
}

// write chunk to given output file stream
void Chunk::write(ofstream& out)
{
	vector<byte> size = toVec( data.size() );

	for(byte elem:size)
		out << elem;
	for (byte elem:name)
		out << elem;
	for (byte elem:data)
		out << elem;
	for (byte elem:crc)
		out << elem;
}

#endif