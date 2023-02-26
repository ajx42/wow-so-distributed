targets=reorder

all: $(targets)

.PHONY: clean

CC=clang
CXX=clang++

flags=

cxxFlags=$(shell llvm-config --cxxflags)
ldFlags=$(shell llvm-config --ldflags --libs)

reorder: reorder.cc
	$(CXX) -o $@ $< $(cxxFlags) $(ldFlags) $(flags)

clean:
	rm $(targets) *.o *.ll

