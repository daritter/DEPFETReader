#ifndef DEPFET_ADCVALUES_H
#define DEPFET_ADCVALUES_H

#include <stdexcept>

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
      bool operator!(){ return m_data.empty(); }
      template<class T2> void substract(const ValueMatrix<T2>& other){
        if(other.getSizeX() != m_sizeX || other.getSizeY() != m_sizeY) {
          throw std::runtime_error("Dimensions do not match");
        }
        for(size_t i=0; i<m_data.size(); ++i) m_data[i] -= (value_type) other[i];
      }
    protected:
      size_t m_sizeX;
      size_t m_sizeY;
      std::vector<value_type> m_data;
  };

  class ADCValues: public ValueMatrix<double> {
    public:
      ADCValues(): ValueMatrix<double>(), m_moduleNr(0) {}
      int getModuleNr() const { return m_moduleNr; }
      int getTriggerNr() const { return m_triggerNr; }
      void setModuleNr(int moduleNr) { m_moduleNr = moduleNr; }
      void setTriggerNr(int triggerNr){ m_triggerNr = triggerNr; }
    protected:
      int m_moduleNr;
      int m_triggerNr;
  };

}

#endif
