#ifndef DEPFET_S3ACONVERTER_H
#define DEPFET_S3ACONVERTER_H

#include <DEPFETReader/RawData.h>
#include <DEPFETReader/ADCValues.h>

namespace DEPFET {

  struct S3AConverter {
    void operator()(const RawData& rawData, ADCValues& adcValues);
  };

}
#endif
