#ifndef DEPFET_S3ACONVERTER_H
#define DEPFET_S3ACONVERTER_H

#include <pxdTestBeam/RawData.h>
#include <pxdTestBeam/ADCValues.h>

namespace DEPFET {

  struct S3AConverter {
    void operator()(const RawData &rawData, ADCValues &adcValues);
  };

}
#endif
