#ifndef DEPFET_DCDCONVERTER_H
#define DEPFET_DCDCONVERTER_H

#include <DEPFETReader/RawData.h>
#include <DEPFETReader/ADCValues.h>

namespace DEPFET {

  struct DCDConverter2Fold {
    DCDConverter2Fold(bool useDCDMapping): m_useDCDMapping(useDCDMapping) {}
    bool m_useDCDMapping;
    void operator()(const RawData& rawData, ADCValues& adcValues);
  };

  struct DCDConverter4Fold {
    DCDConverter4Fold(bool useDCDMapping): m_useDCDMapping(useDCDMapping) {}
    bool m_useDCDMapping;
    void operator()(const RawData& rawData, ADCValues& adcValues);
  };
}
#endif
