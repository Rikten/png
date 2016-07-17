all:
	@g++ -std=c++11 *.cpp -lz

clean:
	@rm a.out

wc:
	@wc *.cpp *.h
