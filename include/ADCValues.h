#ifndef DEPFET_ADCVALUES_H
#define DEPFET_FRAME_H

namespace DEPFET {

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

}

#endif
