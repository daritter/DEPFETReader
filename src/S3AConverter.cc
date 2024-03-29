#include <DEPFETReader/S3AConverter.h>

namespace DEPFET {

  size_t S3AConverter::operator()(const RawData& rawData, ADCValues& adcValues)
  {
    adcValues.setSize(64, 128);
    DataView<unsigned int> data = rawData.getView<unsigned int>();
    for (size_t ipix = 0; ipix < adcValues.getSizeX()*adcValues.getSizeY(); ++ipix) { //-- raspakowka daty ---- loop 8000
      int x = data[ipix] >> 16 & 0x3F;
      int y = data[ipix] >> 22 & 0x7F;
      adcValues(x, y) = data[ipix] & 0xffff;
    }
    return rawData.getFrameSize<unsigned int>(64, 128);
  }

}
