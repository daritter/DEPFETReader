#include <DEPFETReader/S3BConverter.h>

namespace DEPFET {

  void S3BConverter2Fold::operator()(const RawData& rawData, ADCValues& adcValues)
  {
    adcValues.setSize(64, 256);
    DataView<short> data = rawData.getView<short>(128, 128);

    for (int gate = 0; gate < 128; ++gate) {
      int readout_gate = (rawData.getStartGate() + gate) % 128;
      int odderon = readout_gate % 2;
      int rgate = readout_gate * 2;
      for (int col = 0; col < 32; col += 2) {
        adcValues(63 - col, rgate + 1 - odderon) = data(gate, (col * 4) + 0) & 0xffff;
        adcValues(col,      rgate     + odderon) = data(gate, (col * 4) + 1) & 0xffff;
        adcValues(62 - col, rgate + 1 - odderon) = data(gate, (col * 4) + 2) & 0xffff;
        adcValues(col + 1,  rgate     + odderon) = data(gate, (col * 4) + 3) & 0xffff;
        adcValues(63 - col, rgate     + odderon) = data(gate, (col * 4) + 4) & 0xffff;
        adcValues(col,      rgate + 1 - odderon) = data(gate, (col * 4) + 5) & 0xffff;
        adcValues(62 - col, rgate     + odderon) = data(gate, (col * 4) + 6) & 0xffff;
        adcValues(col + 1,  rgate + 1 - odderon) = data(gate, (col * 4) + 7) & 0xffff;
      }
    }
  }

  void S3BConverter4Fold::operator()(const RawData& rawData, ADCValues& adcValues)
  {
    adcValues.setSize(32, 512);
    DataView<short> data = rawData.getView<short>(128, 128);
    for (int gate = 0; gate < 128; ++gate)  {
      int readout_gate = (rawData.getStartGate() + gate) % 128;
      int rgate = readout_gate * 4;
      for (int col = 0; col < 16; col += 1) {
        adcValues(31 - col, rgate + 3) = data(gate, (col * 8) + 0) & 0xffff;
        adcValues(col,      rgate + 0) = data(gate, (col * 8) + 1) & 0xffff;
        adcValues(31 - col, rgate + 2) = data(gate, (col * 8) + 2) & 0xffff;
        adcValues(col,      rgate + 1) = data(gate, (col * 8) + 3) & 0xffff;
        adcValues(31 - col, rgate + 1) = data(gate, (col * 8) + 4) & 0xffff;
        adcValues(col,      rgate + 2) = data(gate, (col * 8) + 5) & 0xffff;
        adcValues(31 - col, rgate + 0) = data(gate, (col * 8) + 6) & 0xffff;
        adcValues(col,      rgate + 3) = data(gate, (col * 8) + 7) & 0xffff;
      }
    }
  }
}
