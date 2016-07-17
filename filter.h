#ifndef FILTER_H
#define FILTER_H

#include <cmath>
#include "utils.h"

/*
	Functions to handle filtering and defiltering of decompressed PNG
	data streams. All filter types for PNG filter method 0 (the only one
	currently defined) are included.

	Notes on filter algorithms:
		line vector = unfiltered scanline
			!!! Calculations that involve a previous byte in the same
				scanline use the RAW (unfiltered) values for those bytes. A
				temp vector is created at the start to hold the original
				scanline for this purpose.
		prev vector = previous (above) unfiltered scanline

	On defilter algorithms:
		line vector = filtered scanline
		prev vector = previous (above) unfiltered scanline
		Calculations that involve the surrounding bytes still use their
		raw (unfiltered) versions, however, as the scanlines are defiltered
		top-to-bottom & left-to-right, there is no need to create a temp holder
		for the current scanline.
*/

///////////////////////////////////////////////////////////////////////////

byte sub(byte raw, byte left);

byte deSub(byte sub, byte left);

void subLine(vector<byte>& line, int bpp);

void deSubLine(vector<byte>& line, int bpp);

///////////////////////////////////////////////////////////////////////////

byte up(byte raw, byte upper);

byte deUp(byte up, byte upper);

void upLine(vector<byte>& line, const vector<byte>& prev);

void deUpLine(vector<byte>& line, const vector<byte>& prev);

///////////////////////////////////////////////////////////////////////////

byte average(byte raw, byte left, byte upper);

byte deAverage(byte average, byte left, byte upper);

void averageLine(vector<byte>& line, const vector<byte>& prev, int bpp);

void deAverageLine(vector<byte>& line, const vector<byte>& prev, int bpp);

///////////////////////////////////////////////////////////////////////////

byte paeth(byte raw, byte left, byte upper, byte upperLeft);

byte dePaeth(byte paeth, byte left, byte upper, byte upperLeft);

void paethLine(vector<byte>& line, const vector<byte>& prev, int bpp);

void dePaethLine(vector<byte>& line, const vector<byte>& prev, int bpp);

byte paethPredictor(byte a, byte b, byte c);

///////////////////////////////////////////////////////////////////////////

int doBestFilter(vector<byte>& line, const vector<byte>& prev, int bpp);

int heuristic(const vector<byte>& line);

///////////////////////////////////////////////////////////////////////////

byte sub(byte raw, byte left)
{
	return raw - left;
}

byte deSub(byte sub, byte left)
{
	return sub + left;
}

void subLine(vector<byte>& line, int bpp)
{
	vector<byte> temp = line;

	for (int x = 0; x < bpp; ++x)	// for x - bpp < 0
		line[x] = sub( line[x], 0x00 );

	for (int x = bpp; x < line.size(); ++x)
		line[x] = sub( line[x], temp[x - bpp] );
}

void deSubLine(vector<byte>& line, int bpp)
{
	for (int x = 0; x < bpp; ++x)	// for x - bpp < 0
		line[x] = deSub( line[x], 0x00 );

	for (int x = bpp; x < line.size(); ++x)
		line[x] = deSub( line[x], line[x - bpp] );
}

byte up(byte raw, byte upper)
{
	return raw - upper;
}

byte deUp(byte up, byte upper)
{
	return up + upper;
}

void upLine(vector<byte>& line, const vector<byte>& prev)
{
	for (int x = 0; x < line.size(); ++x)
		line[x] = up( line[x], prev[x] );
}

void deUpLine(vector<byte>& line, const vector<byte>& prev)
{
	for (int x = 0; x < line.size(); ++x)
		line[x] = deUp( line[x], prev[x] );
}

byte average(byte raw, byte left, byte upper)
{
	return raw - (byte)( ((int)left + (int)upper) / 2 );
}

byte deAverage(byte average, byte left, byte upper)
{
	return average + (byte)( ((int)left + (int)upper) / 2 );
}

void averageLine(vector<byte>& line, const vector<byte>& prev, int bpp)
{
	vector<byte> temp = line;

	for (int x = 0; x < bpp; ++x)	// for x - bpp < 0
		line[x] = average( line[x], 0x00, prev[x] );

	for (int x = bpp; x < line.size(); ++x)
		line[x] = average( line[x], temp[x - bpp], prev[x] );
}

void deAverageLine(vector<byte>& line, const vector<byte>& prev, int bpp)
{
	for (int x = 0; x < bpp; ++x)	// for x - bpp < 0
		line[x] = deAverage( line[x], 0x00, prev[x] );

	for (int x = bpp; x < line.size(); ++x)
		line[x] = deAverage( line[x], line[x - bpp], prev[x] );
}

byte paeth(byte raw, byte left, byte upper, byte upperLeft)
{
	return raw - paethPredictor(left, upper, upperLeft);
}

byte dePaeth(byte paeth, byte left, byte upper, byte upperLeft)
{
	return paeth + paethPredictor(left, upper, upperLeft);
}

void paethLine(vector<byte>& line, const vector<byte>& prev, int bpp)
{
	vector<byte> temp = line;

	for (int x = 0; x < bpp; ++x)	// for x - bpp < 0
		line[x] = paeth( line[x], 0x00, prev[x], 0x00 );

	for (int x = bpp; x < line.size(); ++x)
		line[x] = paeth( line[x], temp[x - bpp], prev[x], prev[x - bpp] );
}

void dePaethLine(vector<byte>& line, const vector<byte>& prev, int bpp)
{
	for (int x = 0; x < bpp; ++x)	// for x - bpp < 0
		line[x] = dePaeth( line[x], 0x00, prev[x], 0x00 );

	for (int x = bpp; x < line.size(); ++x)
		line[x] = dePaeth( line[x], line[x - bpp], prev[x], prev[x - bpp] );
}

byte paethPredictor(byte a, byte b, byte c)
{
	int p = static_cast<int>(a) + static_cast<int>(b) - static_cast<int>(c);

	int pa = abs( p - static_cast<int>(a) );
	int pb = abs( p - static_cast<int>(b) );
	int pc = abs( p - static_cast<int>(c) );

	if ( pa <= pb && pa <= pc )
		return a;
	else if (pb <= pc)
		return b;
	else
		return c;
}

// adaptive filtering algorithm, uses minimum sum heuristic
int doBestFilter(vector<byte>& line, const vector<byte>& prev, int bpp)
{
	int result, h0, h1, h2, h3, h4;

	vector<byte> res1 = line, 
		res2 = line, 
		res3 = line, 
		res4 = line;

	subLine(res1, bpp);
	upLine(res2, prev);
	averageLine(res3, prev, bpp);
	paethLine(res4, prev, bpp);

	h0 = heuristic(line);
	h1 = heuristic(res1);
	h2 = heuristic(res2);
	h3 = heuristic(res3);
	h4 = heuristic(res4);

	result = min( min( min(h1, h2), min(h3, h4) ), h0 );	// smallest heuristic is the best filter type
															// for this line
	if      (result == h0)
		return 0;
	else if (result == h1)
	{
		line = res1;
		return 1;
	}
	else if (result == h2)
	{
		line = res2;
		return 2;
	}
	else if (result == h3)
	{
		line = res3;
		return 3;
	}
	else
	{
		line = res4;
		return 4;
	}
}

int heuristic(const vector<byte>& line)
{
	int result = 0;

	for (byte elem:line)
		result += abs( static_cast<int>( static_cast<char>(elem) ) );

	return result;
}

#endif