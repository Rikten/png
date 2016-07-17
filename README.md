#About
An decoder/encoder for png images written in c++.

External libs used in this project:
* zlib, for compression/decompression
* some small code from libpng.org for crc-32 calculations

##Run
```bash
git clone https://github.com/Rikten/png.git
make
./a.out [options] filename
```

##Todo
* Re-implement image display using OpenGL (a pre-github version did this using glut (now deprecated) for context creation)
* Split into .h/.cpp (Now that I'm using git, I have no excuse to be messy)
* Support for some auxiliary chunks
