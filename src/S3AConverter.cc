#include <pxdTestBeam/S3AConverter.h>

namespace DEPFET {

  void S3AConverter::operator()(const RawData &rawData, ADCValues &adcValues)
  {
    DataView<unsigned int> data = rawData.getView<unsigned int>();
    for (size_t ipix = 0; ipix < adcValues.getNX()*adcValues.getNY(); ++ipix) { //-- raspakowka daty ---- loop 8000
      int x = data[ipix] >> 16 & 0x3F;
      int y = data[ipix] >> 22 & 0x7F;
      adcValues(x,y) = data[ipix] & 0xffff;
    }
  }

}
