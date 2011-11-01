#ifndef DEPFET_COMMONMODE_H
#define DEPFET_COMMONMODE_H

#include <DEPFETReader/IncrementalMean.h>
#include <DEPFETReader/ADCValues.h>
#include <vector>
#include <cmath>

namespace DEPFET {
  class CommonMode {
    public:
      CommonMode():m_nX(0),m_nY(0){}
      void calculate(const ADCValues &data, const ValueMatrix<IncrementalMean>& pedestals, double nSigma);
      double operator()(int x, int y){
        int xhalf = 2*x/m_nX;
        return m_commonModeX[x].getMean() + m_commonModeY[(xhalf*m_nY + y)/2].getMean();
      }
    protected:
      int m_nX;
      int m_nY;
      std::vector<IncrementalMean> m_commonModeX;
      std::vector<IncrementalMean> m_commonModeY;
  };

  inline void CommonMode::calculate(const ADCValues &data, const ValueMatrix<IncrementalMean>& pedestals, double nSigma){
    m_nX = data.getSizeX();
    m_nY = data.getSizeY();
    m_commonModeX.clear();
    m_commonModeX.resize(m_nX);
    m_commonModeY.clear();
    m_commonModeY.resize(m_nY);

    for(int dir=0; dir<2; ++dir){
      for(int x=0; x<m_nX; ++x){
        int xhalf = 2*x/m_nX;
        for(int y=0; y<m_nY; ++y){
          double signal = data(x,y) - pedestals(x,y).getMean();
          if(std::fabs(signal)>nSigma*pedestals(x,y).getSigma()) continue;

          IncrementalMean &cmX =  m_commonModeX[x];
          IncrementalMean &cmY =  m_commonModeY[(xhalf*m_nY + y)/2];
          if(dir==1){
            cmX.add(signal-cmY.getMean());
          }else{
            cmY.add(signal-cmX.getMean());
          }
        }
      }
    }
  }
}
#endif
