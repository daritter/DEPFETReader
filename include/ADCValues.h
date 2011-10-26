#ifndef DEPFET_ADCVALUES_H
#define DEPFET_ADCVALUES_H

namespace DEPFET {

  template<class T> class ValueMatrix {
    public:
      typedef T value_type;
      ValueMatrix():m_sizeX(0), m_sizeY(0) {}
      ValueMatrix(size_t sizeX, size_t sizeY):m_sizeX(sizeX),m_sizeY(sizeY),m_data(sizeX*sizeY) {}
      void setSize(size_t sizeX, size_t sizeY){ m_sizeX=sizeX; m_sizeY=sizeY; clear(); }
      template<class T2> void setSize(const ValueMatrix<T2>& other){ setSize(other.getSizeX(),other.getSizeY()); }
      void clear(){ m_data.clear(); m_data.resize(m_sizeX*m_sizeY); }
      size_t getSizeX() const { return m_sizeX; }
      size_t getSizeY() const { return m_sizeY; }
      size_t getSize() const { return m_data.size(); }
      value_type operator()(size_t x, size_t y) const { return m_data[x*m_sizeY+y]; }
      value_type operator[](size_t index) const { return m_data[index]; }
      value_type& operator()(size_t x, size_t y) { return m_data[x*m_sizeY+y]; }
      value_type& operator[](size_t index){ return m_data[index]; }
    protected:
      size_t m_sizeX;
      size_t m_sizeY;
      std::vector<value_type> m_data;
  };

  class ADCValues: public ValueMatrix<int> {
    public:
      ADCValues(): ValueMatrix<int>(), m_moduleNr(0) {}
      int getModuleNr() const { return m_moduleNr; }
      void setModuleNr(int moduleNr) { m_moduleNr = moduleNr; }
    protected:
      int m_moduleNr;
  };

}

#endif
