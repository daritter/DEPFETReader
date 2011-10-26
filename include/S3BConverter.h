#ifndef DEPFET_S3BCONVERTER_H
#define DEPFET_S3BCONVERTER_H

#include <DEPFETReader/RawData.h>
#include <DEPFETReader/ADCValues.h>

namespace DEPFET {

  struct S3BConverter2Fold {
    void operator()(const RawData &rawData, ADCValues &adcValues);
  };

  struct S3BConverter4Fold {
    void operator()(const RawData &rawData, ADCValues &adcValues);
  };
}
#endif
