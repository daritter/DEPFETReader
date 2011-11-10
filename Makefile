SOURCES = $(wildcard src/*.cc)
CXXFLAGS = -O2

all: depfetConverter

depfetConverter: tools/depfetConverter.cc $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^ -I. -lboost_program_options

depfetReader: tools/depfetReader.cc $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^ -I. $(shell root-config --cflags --ldflags --libs)

$(SOURCES): DEPFETReader

DEPFETReader:
	ln -sfT include DEPFETReader

