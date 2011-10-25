#ifndef DEPFET_RAWDATA_H
#define DEPFET_RAWDATA_H

#include <vector>
#include <stdexcept>
#include <istream>

namespace DEPFET {

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

  class RawData {
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

}
#endif
