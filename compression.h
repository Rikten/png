#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "utils.h"
#include "zlib.h"

const int CHUNK = 32768;

/*
Decompression function that uses zlib. Code is originally from the
zpipe.c example that comes with zlib, modified to be used with the
more modern vector container as opposed to c-style FILE* structs.
Consider cleaning this up, or converting to a void function.
*/
int inf(const vector<byte>& source, vector<byte>& result)
{
    int ret;
    unsigned have;
    z_stream strm;
    byte in[CHUNK];
    byte out[CHUNK];

    int srcIndex = 0;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    result.clear();

    ret = inflateInit(&strm);

    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        // copy bytes from source vector to c-array
        for (int x = 0; (x < CHUNK) && (srcIndex < source.size()); ++x, ++srcIndex, ++strm.avail_in)
        	in[x] = source[srcIndex];

        if (strm.avail_in == 0)
            break;

        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;

            // copy inflated data from c-array to result vector
            for (int x = 0; x < have; ++x)
            	result.push_back( out[x] );

        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/*
Compression function, also modified from zlib example zlib.h
*/
int def(const vector<byte>& source, vector<byte>& result, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    byte in[CHUNK];
    byte out[CHUNK];

    int srcIndex = 0;

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0; // lel

    result.clear(); // we want to start with an empty result vector

    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        // copy bytes from source vector to c-array
        for (int x = 0; (x < CHUNK) && (srcIndex < source.size()); ++x, ++srcIndex, ++strm.avail_in)
            in[x] = source[srcIndex];

        flush = ( srcIndex == source.size() ) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;

            // write back to result vector
            for (int x = 0; x < have; ++x)
                result.push_back( out[x] );

        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while ( srcIndex < source.size() );
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

#endif