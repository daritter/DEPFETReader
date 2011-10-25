#include <pxdTestBeam/S3BConverter.h>

namespace DEPFET {

  void S3BConverter2Fold::operator()(const RawData &rawData, ADCValues &adcValues)
  {
    DataView<short> data = rawData.getView<short>(128, 128);

    for (int gate = 0; gate < 128; ++gate) {
      int readout_gate = (rawData.getStartGate() + gate) % 128;
      int odderon = readout_gate % 2;
      int rgate = readout_gate * 2;
      for (int col = 0; col < 128; col += 8) {
        adcValues(63 - col, rgate + 1 - odderon) = data(gate, col + 0) & 0xffff;
        adcValues(col,      rgate     + odderon) = data(gate, col + 1) & 0xffff;
        adcValues(62 - col, rgate + 1 - odderon) = data(gate, col + 2) & 0xffff;
        adcValues(col + 1,  rgate     + odderon) = data(gate, col + 3) & 0xffff;
        adcValues(63 - col, rgate     + odderon) = data(gate, col + 4) & 0xffff;
        adcValues(col,      rgate + 1 - odderon) = data(gate, col + 5) & 0xffff;
        adcValues(62 - col, rgate     + odderon) = data(gate, col + 6) & 0xffff;
        adcValues(col + 1,  rgate + 1 - odderon) = data(gate, col + 7) & 0xffff;
      }
    }
  }

  void S3BConverter4Fold::operator()(const RawData &rawData, ADCValues &adcValues)
  {
    DataView<short> data = rawData.getView<short>(128, 128);
    for (int gate = 0; gate < 128; ++gate)  {
      int readout_gate = (rawData.getStartGate() + gate) % 128;
      int rgate = readout_gate * 4;
      for (int col = 0; col < 128; col += 8) {
        adcValues(31 - col, rgate + 3) = data(gate, col + 0) & 0xffff;
        adcValues(col,      rgate + 0) = data(gate, col + 1) & 0xffff;
        adcValues(31 - col, rgate + 2) = data(gate, col + 2) & 0xffff;
        adcValues(col,      rgate + 1) = data(gate, col + 3) & 0xffff;
        adcValues(31 - col, rgate + 1) = data(gate, col + 4) & 0xffff;
        adcValues(col,      rgate + 2) = data(gate, col + 5) & 0xffff;
        adcValues(31 - col, rgate + 0) = data(gate, col + 6) & 0xffff;
        adcValues(col,      rgate + 3) = data(gate, col + 7) & 0xffff;
      }
    }
  }
}
