#include <DEPFETReader/DCDConverter.h>
namespace DEPFET {

  int SWBChannelMap[16]   = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

  int FPGAToDrainMap[128] = {
    0, 4, 8, 12, 2, 6, 10, 14, 124, 120,
    116, 112, 126, 122, 118, 114, 16, 20, 24, 28,
    18, 22, 26, 30, 108, 104, 100, 96, 110, 106,
    102, 98, 36, 35, 37, 34, 52, 51, 53, 50,
    68, 67, 69, 66, 84, 83, 85, 82, 38, 33,
    39, 32, 54, 49, 55, 48, 70, 65, 71, 64,
    86, 81, 87, 80,  1, 5, 9, 13, 3, 7,
    11, 15, 125, 121, 117, 113, 127, 123, 119, 115, 17,
    21, 25, 29, 19, 23, 27, 31, 109, 105,
    101, 97, 111, 107, 103, 99, 44, 43, 45,
    42, 60, 59, 61, 58, 76, 75, 77, 74, 92,
    91, 93, 90, 46, 41, 47, 40, 62, 57, 63,
    56, 78, 73, 79, 72, 94, 89, 95, 88
  };

  void DCDConverter2Fold::operator()(const RawData& rawData, ADCValues& adcValues)
  {
    adcValues.setSize(64, 32);
    DataView<signed char> v4data = rawData.getView<signed char>();
    if (m_useDCDMapping) {
      // printf("=> try with internal maps \n");
      // do not touch maps above!! and cross fingers

      int iData = -1;  // pointer to raw data
      int noOfDCDBChannels = adcValues.getSizeX() * 2; // used channels only
      int noOfSWBChannels = adcValues.getSizeY() / 2;  // used channels only

      for (int offset = 0; offset < noOfSWBChannels; ++offset)  { // loop over SWB channels
        // this is the SWB channel/pad switched on
        int iSWB = (rawData.getStartGate() + offset) % (noOfSWBChannels);
        // which is bonded to PXD5 double row number
        int iDoubleRow = SWBChannelMap[iSWB];

        for (int iFPGA = 0; iFPGA < noOfDCDBChannels; ++iFPGA) { // loop over DCDB channels
          // position iFPGA in serial output is mapped to drain line number
          int iDrain = FPGAToDrainMap[iFPGA];
          // irow and icol are referring to physical pixels on sensor
          int icol, irow;
          icol = iDrain / 2;

          // double rows are counted 0,1,2,... starting from DCDB and first
          // connected double row is even!!
          if (iDoubleRow % 2 == 0) {
            // ok, this double row is even
            if (iDrain % 2 == 0) irow = 2 * iDoubleRow;
            else irow = 2 * iDoubleRow + 1;
          } else  {
            // ok, this double row is odd
            if (iDrain % 2 == 0) irow = 2 * iDoubleRow + 1 ;
            else irow = 2 * iDoubleRow;
          }

          adcValues(icol, irow) = (signed short) v4data[++iData];
        }
      }
    } else {
      // all encodings done on daq (only bonn laser data)
      int ipix = -1;
      for (size_t offset = 0; offset < adcValues.getSizeY(); ++offset)  { // loop over Switcher channels
        int igate = (rawData.getStartGate() + offset) % adcValues.getSizeY();
        for (size_t idrain = 0; idrain < adcValues.getSizeX(); ++idrain) { // loop over DCD channels
          adcValues(idrain, igate) = (signed short) v4data[++ipix];
        }
      }
    }
  }

  void DCDConverter4Fold::operator()(const RawData& rawData, ADCValues& adcValues)
  {
    adcValues.setSize(32, 64);
    DataView<signed char> v4data = rawData.getView<signed char>();
    int iPix(-1);
    int nGates = adcValues.getSizeY() / 4;
    int nColDCD = adcValues.getSizeX() * 4;
    for (int gate = 0; gate < nGates; ++gate) {
      int iRowD1 = (rawData.getStartGate() + gate) % nGates;
      for (int colDCD = 0; colDCD < nColDCD; ++colDCD) {
        int icolD = m_useDCDMapping ? FPGAToDrainMap[colDCD] : colDCD;
        int col = (icolD / 4) % adcValues.getSizeX();
        int row = (iRowD1 * 4 + 3 - icolD % 4) % adcValues.getSizeY();
        adcValues(col, row) = (signed short) v4data[++iPix];
      }
    }
  }
}
