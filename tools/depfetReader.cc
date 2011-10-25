#include <pxdTestBeam/FileReader.h>

int main(int argc, char* argv[])
{
  DEPFET::FileReader reader(argv[1], 0, std::cout);
  reader.readGroupHeader();
  reader.readHeader();
  reader.readEventHeader(0);
  int eventsize;
  reader.readEvent(0, eventsize);
  return 0;
}
