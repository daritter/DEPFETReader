SOURCES = $(wildcard src/*.cc)
CXXFLAGS = -O2

ALL = depfetCalibration depfetHitmap depfetDump

all: $(ALL)

$(ALL): %: tools/%.cc $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^ -I. -lboost_program_options  $(shell root-config --cflags --ldflags --libs)

$(SOURCES): DEPFETReader

DEPFETReader:
	ln -sfT include DEPFETReader

clean:
	rm -f DEPFETReader $(ALL)

.PHONY: clean
