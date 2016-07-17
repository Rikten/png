#ifndef PNG_H
#define PNG_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <array>
#include <vector>

#include "utils.h"
#include "crc.h"
#include "compression.h"
#include "filter.h"
#include "Chunk.h"

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::min;
using std::string;
using std::array;
using std::vector;

typedef vector<vector<vector<byte>>> byteMatrix;

const array<byte, 8> PNG_HEADER = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

const vector<string> KNOWN_CHUNKS = {"IHDR", "IDAT", "IEND"};

class PNG
{
private:
	int mWidth, mHeight;
	int bitDepth, colorType;
	int compressionMethod, filterMethod, interlaceMethod;
	int mBytesPerPixel;

	vector<Chunk> chunks;

	byteMatrix mImage;

	void decode();
	void encode();

	void readIHDR();
	void readPLTE();

	vector<byte> decompress(const vector<byte>& deflatedData);
	byteMatrix defilter(const vector<byte>& inflatedData);

	vector<byte> filter(const byteMatrix& image);
	vector<byte> compress(const vector<byte>& filteredData);
	
public:
	void load(string f);
	void save(string f);

	void invert();
	void simplify();

	void display();
	void printInfo();
};

// simplify raw data of png if any reductions can be made:
// RGB to grayscale, if RGB samples in each pixel are the same
// TODO: remove alpha channel if all pixels are fully opaque
void PNG::simplify()
{
	bool grayScale;

	if (colorType == 2 || colorType == 6)
	{
		grayScale = true;
		for (int i = 0; i < mHeight; ++i)
			for (int j = 0; j < mWidth; ++j)
				if ( !allSame(mImage[i][j], 3) )
					grayScale = false;

		if (grayScale)
		{
			for (int i = 0; i < mHeight; ++i)
				for (int j = 0; j < mWidth; ++j)
					mImage[i][j].erase( mImage[i][j].begin(), mImage[i][j].begin() + 2 );

			colorType = (colorType == 2) ? 0 : 4;
			mBytesPerPixel = (colorType == 0) ? 1 : 2;

			cout << "RGB values in each pixel were identical. Image has been converted to grayscale.\n\n";
		}
	}
}

// invert RGB (or grayscale) values across whole image
// alpha value is not inverted
void PNG::invert()
{
	int p = mImage[0][0].size(); // which samples to invert (skip alpha)

	if      (p == 1 || p == 2)
		p = 1;
	else if (p == 3 || p == 4)
		p = 3;

	for (int i = 0; i < mHeight; ++i)
		for (int j = 0; j < mWidth; ++j)
			for (int k = 0; k < p; ++k)
				mImage[i][j][k] = 0xFF - mImage[i][j][k];
}

// load an image with filename f, first as chunks, and then
// decode the image into a matrix of pixels
// the function does check for bad file input
// TODO: maybe give an option for whether or not the user wants output?
void PNG::load(string f)
{
	array<byte, 4> tempSize;
	unsigned int nextChunkSize;

	int fileSize;

	Chunk tempC;

	ifstream reader;

	reader.open(f);

	// error checking for bad filename
	if ( reader.fail() )
		quit("The file \"" + f + "\" does not exist.\n");

	// get size of file (bytes)
	reader.seekg(0, reader.end);
	fileSize = reader.tellg();
	reader.seekg(0, reader.beg);

	for (byte elem:PNG_HEADER)
		if ( elem != reader.get() )
			quit("File header does not match the PNG specification.\n");

	// read all chunks into vector
	cout << "Begin read of file...\n\n";
	while (reader.tellg() < fileSize)
	{
		// figure out size of next chunk's data
		for (int x = 0; x < 4; ++x)
			tempSize[x] = reader.get();

		nextChunkSize = toUInt(tempSize);

		// get various components of the chunk
		for (int x = 0; x < 4; ++x)
			tempC.addNameByte( reader.get() );

		for (int x = 0; x < nextChunkSize; ++x)
			tempC.addDataByte( reader.get() );

		for (int x = 0; x < 4; ++x)
			tempC.addCrcByte( reader.get() );

		// check for correct crc, terminate if violation is found
		if ( crc( tempC.getCrcData() ) != toUInt( tempC.getCrc() ) )
			quit("Bad checksum. The file appears to be corrupted.\n");

		// is this an unrecognized critical chunk? if so, image cannot be reliably
		// read, program must terminate
		if ( !contains(KNOWN_CHUNKS, toString(tempC.getName())) && !tempC.isAncillary() )
			quit("Unknown critical chunk " + toString(tempC.getName()) + " encountered.\n");

		// add this chunk to the stream if chunk is known/safe-copy bit is high
		// consider rewriting to avoid the need for reset()
		if ( contains(KNOWN_CHUNKS, toString(tempC.getName())) || tempC.isSafeToCopy() )
			chunks.push_back(tempC);
		else
			cout << "Unrecognized, unsafe-to-copy chunk " << toString(tempC.getName()) << " discarded.\n";

		tempC.reset();
	}

	reader.close();

	cout << "File loaded into memory. This file is " << dec << fileSize << " bytes long.\n";
	cout << "The file has " << chunks.size() << " different chunks.\n\n";

	// decode image!
	decode();
}

void PNG::save(string f)
{
	vector<Chunk> toWrite;
	Chunk nextChunk;

	int fileSize;

	vector<byte> filteredData;
	vector<byte> deflatedData;

	ofstream writer;

	// filter and compress 
	filteredData = filter(mImage);
	deflatedData = compress(filteredData);

	// create new IHDR chunk ------------------------------------------
	// TODO: make name set by string?, make this a function
	nextChunk.setName( { 0x49, 0x48, 0x44, 0x52 } );
	vector<byte> nextVal;

	// IHDR Data mWidth/mHeight
	nextVal = toVec(mWidth);
	for (byte elem:nextVal)
		nextChunk.addDataByte(elem);

	nextVal = toVec(mHeight);
	for (byte elem:nextVal)
		nextChunk.addDataByte(elem);
	nextChunk.addDataByte(0x08);

	nextChunk.addDataByte( (byte)colorType );
	nextChunk.addDataByte(0); // compression
	nextChunk.addDataByte(0); // filter
	nextChunk.addDataByte(0); // interlace

	// IHDR checksum
	nextChunk.setCrc( toVec( crc( nextChunk.getCrcData() ) ) );

	toWrite.push_back(nextChunk);

	// put other writable chunks into output chunk stream
	// should check safe-to-copy on unrecognized chunks

	// create IDAT ------------------------------------------------
	nextChunk.reset();
	nextChunk.setName( { 0x49, 0x44, 0x41, 0x54 } );

	nextChunk.setData(deflatedData);
	nextChunk.setCrc( toVec( crc( nextChunk.getCrcData() ) ) );

	toWrite.push_back(nextChunk);

	// create IEND ------------------------------------------------
	nextChunk.reset();
	nextChunk.setName( { 0x49, 0x45, 0x4E, 0x44 } );

	nextChunk.setCrc( { 0xAE, 0x42, 0x60, 0x82 } );

	toWrite.push_back(nextChunk);

	writer.open(f);

	// write PNG header to file
	for (byte elem:PNG_HEADER)
		writer << elem;

	// write chunks to file
	for (Chunk elem:toWrite)
		elem.write(writer);

	fileSize = writer.tellp();
	writer.close();

	cout << "Wrote image to " << f << "\n";
	cout << "Written image is " << fileSize << " bytes long.\n";
}

// decode image from a set of chunks to a pixel matrix
void PNG::decode()
{
	vector<byte> idatContents;
	vector<byte> inflatedData;

	readIHDR();

	// concatenate contents of all IDAT chunks
	for (Chunk elem:chunks)
		if ( toString( elem.getName() ) == "IDAT" )
			for ( byte data:elem.getData() )
				idatContents.push_back(data);

	// decompress/defilter all bytes from IDAT
	inflatedData = decompress(idatContents);
	mImage = defilter(inflatedData);
}

// create chunk vector to be written to png file
void PNG::encode()
{
	// figure out color type of image, reduce raw data if necessary

	// filter raw data

	// compress filtered

	// create IHDR chunk

	// copy any existing ancillary chunks if safe to do so

	// write chunks to png file
}

// read the critical IHDR chunk to get basic information about
// the image
void PNG::readIHDR()
{
	// chunk 0 will always be IHDR
	vector<byte> data = chunks[0].getData();

	// get basic image data
	mWidth = toUInt( {data[0], data[1], data[2], data[3]} );
	mHeight = toUInt( {data[4], data[5], data[6], data[7]} );

	// TODO: error checking
	bitDepth = static_cast<int>(data[8]);
	colorType = static_cast<int>(data[9]);
	compressionMethod = static_cast<int>(data[10]);
	filterMethod = static_cast<int>(data[11]);
	interlaceMethod = static_cast<int>(data[12]);

	// Adam7 currently not supported
	if (interlaceMethod == 1)
		quit("The specified image is interlaced. This is currently unsupported.\n");

	// figure out number of bytes per pixel
	// TODO: add more combinations
	if      (colorType == 0)
		mBytesPerPixel = bitDepth / 8; // TODO: mBytesPerPixel < 1
	else if (colorType == 2)
		mBytesPerPixel = bitDepth / 8 * 3;
	else if (colorType == 3)
		;
	else if (colorType == 4)
		mBytesPerPixel = bitDepth / 8 * 2;
	else if (colorType == 6)
		mBytesPerPixel = bitDepth / 8 * 4;
}

// decompress the IDAT chunk
// first step to decoding a PNG image
// (see compression.h)
vector<byte> PNG::decompress(const vector<byte>& deflatedData)
{
	vector<byte> result;

	// inflate the given byte stream (thx zlib)
	inf(deflatedData, result);

	cout << "IDAT has been decompressed.\n"
		<< "Decompressed size is " << result.size() << " bytes.\n"
		<< "Compression factor of "
		<< static_cast<double>(result.size()) / deflatedData.size() << "\n\n";

	return result;
}

// defilter the decompressed data stream
// only 1 filter method currently exists, which in itself has a total
// of 4 + 1 filter types (various + no filter)
// (see filter.h)
byteMatrix PNG::defilter(const vector<byte>& inflatedData)
{
	byteMatrix result;
	vector<byte> pixels;

	int lineSize = mWidth * mBytesPerPixel;

	int pixIndex;

	int nextFilterType;
	bool used0 = false,
		used1 = false,
		used2 = false,
		used3 = false,
		used4 = false;

	// containers to hold current and previous scan lines
	vector<byte> currScanLine;
	vector<byte> prevScanLine;

	currScanLine.resize(lineSize);

	// previous scan line starts as all 0x00s
	for (int x = 0; x < lineSize; ++x)
		prevScanLine.push_back(0x00);

	// defilter each scanline
	// on each pass, the filter type byte is read and then discarded
	for (int infIndex = 0; infIndex < inflatedData.size(); ++infIndex)
	{
		nextFilterType = inflatedData[infIndex];
		if (nextFilterType == 0)
			used0 = true;
		else if (nextFilterType == 1)
			used1 = true;
		else if (nextFilterType == 2)
			used2 = true;
		else if (nextFilterType == 3)
			used3 = true;
		else if (nextFilterType == 4)
			used4 = true;

		// load the next scanline
		for (int x = 0; x < lineSize; ++x)
		{
			++infIndex;
			currScanLine[x] = inflatedData[infIndex];
		}

		// defilter current scan line based on type
		if      (nextFilterType == 0)
			; // no defilter (do nothing)
		else if (nextFilterType == 1)
			deSubLine(currScanLine, mBytesPerPixel);
		else if (nextFilterType == 2)
			deUpLine(currScanLine, prevScanLine);
		else if (nextFilterType == 3)
			deAverageLine(currScanLine, prevScanLine, mBytesPerPixel);
		else if (nextFilterType == 4)
			dePaethLine(currScanLine, prevScanLine, mBytesPerPixel);

		// add the defiltered scan line to raw pixel stream
		for (byte elem:currScanLine)
			pixels.push_back(elem);

		// save the defiltered scanline in case it is needed
		// for the next line's filter type
		prevScanLine = currScanLine;
	}

	cout << "Inflated data has been defiltered.\n"
		<< "Defiltered size is " << pixels.size() << " bytes.\n"
		<< "Types used: "
		<< (used0 ? "0 " : "" )
		<< (used1 ? "1 " : "" )
		<< (used2 ? "2 " : "" )
		<< (used3 ? "3 " : "" )
		<< (used4 ? "4"  : "" )
		<< "\n\n";

	// convert raw pixel stream to pixel matrix
	// resize matrix according to mWidth, mHeight, and bytes per pixel
	result.resize(mHeight);
	for (int i = 0; i < mHeight; ++i)
		result[i].resize(mWidth);
	for (int i = 0; i < mHeight; ++i)
		for (int j = 0; j < mWidth; ++j)
			result[i][j].resize(mBytesPerPixel);

	// transfer pixels to matrix
	pixIndex = 0;
	for (int i = 0; i < mHeight; ++i)
		for (int j = 0; j < mWidth; ++j)
			for (int k = 0; k < mBytesPerPixel; ++k, ++pixIndex)
				result[i][j][k] = pixels[pixIndex];

	return result;
}

vector<byte> PNG::filter(const byteMatrix& image)
{
	vector<byte> result;
	vector<byte> pixels;

	int lineSize = mWidth * mBytesPerPixel;
	int nextFilterType;
	bool used0 = false,
		used1 = false,
		used2 = false,
		used3 = false,
		used4 = false;

	vector<byte> currScanLine;
	vector<byte> prevScanLine;
	vector<byte> temp;

	currScanLine.resize(lineSize);

	for (int x = 0; x < lineSize; ++x)	// previous scan line should start as all 0x00
		prevScanLine.push_back(0x00);

	for (int i = 0; i < mHeight; ++i)
		for (int j = 0; j < mWidth; ++j)
			for (int k = 0; k < mBytesPerPixel; ++k)
				pixels.push_back( image[i][j][k] );

	int pixIndex = 0;
	for (int i = 0; i < mHeight; ++i)
	{
		for (int x = 0; x < lineSize; ++x, ++pixIndex)
			currScanLine[x] = pixels[pixIndex];

		temp = currScanLine;	// unfiltered bytes need to be preserved for some types

		nextFilterType = doBestFilter(currScanLine, prevScanLine, mBytesPerPixel);
		if (nextFilterType == 0)
			used0 = true;
		else if (nextFilterType == 1)
			used1 = true;
		else if (nextFilterType == 2)
			used2 = true;
		else if (nextFilterType == 3)
			used3 = true;
		else if (nextFilterType == 4)
			used4 = true;

		result.push_back( static_cast<byte>(nextFilterType) );

		for (byte elem:currScanLine)
			result.push_back(elem);

		prevScanLine = temp;
	}

	cout << "Raw data has been filtered.\n"
		<< "Filtered size is " << result.size() << " bytes.\n"
		<< "Types used: "
		<< (used0 ? "0 " : "" )
		<< (used1 ? "1 " : "" )
		<< (used2 ? "2 " : "" )
		<< (used3 ? "3 " : "" )
		<< (used4 ? "4"  : "" )
		<< "\n\n";

	return result;
}

vector<byte> PNG::compress(const vector<byte>& filteredData)
{
	vector<byte> result;

	def(filteredData, result, 9);	// 9 = max compression level in zlib

	cout << "Raw data has been compressed.\n"
		<< "Compressed size is " << result.size() << " bytes.\n"
		<< "Compression factor of "
		<< static_cast<double>(filteredData.size()) / result.size() << "\n\n";

	return result;
}

// display image on screen
// WIP: rewrite the openGL to use functions that aren't deprecated
void PNG::display()
{
	cout << "Display function is on vacation. Check back later...\n\n";
}

// print some basic information about the image
void PNG::printInfo()
{
	cout << "Dimensions: " << mWidth << 'x' << mHeight << endl
		<< "Bit depth/color type: " << bitDepth << '/' << colorType << endl
		<< "Bytes per pixel: " << mBytesPerPixel << endl
		<< "Compression/filter/interlace method: "
		<< compressionMethod << '/'
		<< filterMethod << '/'
		<< interlaceMethod << "\n\n";
}

#endif