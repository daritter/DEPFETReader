#include <pxdTestBeam/FileReader.h>
#include <vector>
#include <stdexcept>

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

  template < class VIEWTYPE, class STORAGETYPE = unsigned int > class DataView {
    public:
      DataView(const std::vector<STORAGETYPE>& data, size_t nX, size_t nY) {
        if(nY == 0 && nX>0){
          nY = data.size()*sizeof(STORAGETYPE)/(nX*sizeof(VIEWTYPE));
        }else if(nX == 0 && nY>0){
          nX = data.size()*sizeof(STORAGETYPE)/(nY*sizeof(VIEWTYPE));
        }else if(nX>0 && nY>0 && (nX*nY*sizeof(VIEWTYPE) > data.size() * sizeof(STORAGETYPE))) {
          throw std::runtime_error("Data buffer not large enough");
        }
        m_data = (VIEWTYPE*) & data.front();
        m_nX = nX;
        m_nY = nY;
      }
      const VIEWTYPE& operator()(size_t x, size_t y) const { return m_data[x*m_nY+y]; }
      const VIEWTYPE& operator[](size_t index) const { return m_data[index]; }

      size_t getNX() const { return m_nX; }
      size_t getNY() const { return m_nY; }
    protected:
      VIEWTYPE* m_data;
      size_t m_nX;
      size_t m_nY;
  };

  class Frame {
    public:
      template<class T> DataView<T> getView(size_t nX=0, size_t nY=0) const {
        return DataView<T>(m_data, nX, nY);
      };

      void read(std::istream &in, size_t eventSize) {
        m_data.resize(eventSize);
        in.read((char*) &m_data.front(), sizeof(unsigned int)*eventSize);
      }

      int getStartGate() const { return m_startGate; }
    protected:
      int m_startGate;
      std::vector<unsigned int> m_data;
  };

  class ADCValues {
    public:
      ADCValues(size_t nX, size_t nY): m_nX(nX), m_nY(nY), m_data(nX*nY) {}
      size_t getNX() const { return m_nX; }
      size_t getNY() const { return m_nY; }
      int& operator()(size_t x, size_t y) { return m_data[x*m_nY+y]; }
    protected:
      size_t m_nX;
      size_t m_nY;
      std::vector<int> m_data;
  };

  struct S3AConverter {
    void operator()(const Frame &rawData, ADCValues &adcValues) {
      DataView<unsigned int> data = rawData.getView<unsigned int>();
      for (size_t ipix = 0; ipix < adcValues.getNX()*adcValues.getNY(); ++ipix) { //-- raspakowka daty ---- loop 8000
        int x = data[ipix] >> 16 & 0x3F;
        int y = data[ipix] >> 22 & 0x7F;
        adcValues(x,y) = data[ipix] & 0xffff;
      }
    }
  };

  struct S3BConverter2Fold {
    void operator()(const Frame &rawData, ADCValues &adcValues) {
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
  };

  struct S3BConverter4Fold {
    void operator()(const Frame &rawData, ADCValues &adcValues) {
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
  };

  struct DCDConverter2Fold {
    DCDConverter2Fold(bool useDCDMapping):m_useDCDMapping(useDCDMapping){}
    bool m_useDCDMapping;
    void operator()(const Frame &rawData, ADCValues &adcValues) {
      DataView<signed char> v4data = rawData.getView<signed char>();
      if(m_useDCDMapping){
        // printf("=> try with internal maps \n");
        // do not touch maps above!! and cross fingers

        int iData = -1;  // pointer to raw data
        int noOfDCDBChannels = adcValues.getNX() * 2; // used channels only
        int noOfSWBChannels = adcValues.getNY() / 2;  // used channels only

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

            adcValues(icol,irow) = (signed short) v4data[++iData];
          }
        }
      }else{
        // all encodings done on daq (only bonn laser data)
        int ipix = -1;
        for (size_t offset = 0; offset < adcValues.getNY(); ++offset)  { // loop over Switcher channels
          int igate = (rawData.getStartGate() + offset) % adcValues.getNY();
          for (size_t idrain = 0; idrain < adcValues.getNX(); ++idrain) { // loop over DCD channels
            adcValues(idrain,igate) = (signed short) v4data[++ipix];
          }
        }
      }
    }
  };

  struct DCDConverter4Fold {
    DCDConverter4Fold(bool useDCDMapping):m_useDCDMapping(useDCDMapping){}
    bool m_useDCDMapping;
    void operator()(const Frame &rawData, ADCValues &adcValues) {
      DataView<signed char> v4data = rawData.getView<signed char>();
      int iPix(-1);
      int nGates = adcValues.getNX()/4;
      int nColDCD = adcValues.getNY()*4;
      for(int gate=0; gate<nGates; ++gate){
        int iRowD1 = (rawData.getStartGate() + gate) % nGates;
        for(int colDCD=0; colDCD< nColDCD; ++colDCD){
          int icolD = m_useDCDMapping?FPGAToDrainMap[colDCD]:colDCD;
          int col = icolD/4 % adcValues.getNX();
          int row = (iRowD1*4+icolD%4) % adcValues.getNY();
          adcValues(col,row) = (signed short) v4data[++iPix];
        }
      }
    }
  };
}

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
