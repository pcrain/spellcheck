all:
	g++ -fpermissive -std=c++11 -ljsoncpp -o spellcheck spellcheck.cpp

clean:
	rm -f spellcheck
