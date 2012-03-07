#ifndef DEPFET_COMMONMODE_H
#define DEPFET_COMMONMODE_H

#include <vector>
#include <algorithm>

#include <DEPFETReader/ADCValues.h>

namespace DEPFET {

  /** Class to calculate and apply common mode corrections to raw depfet data.
   *
   * To accomodate for readout electronics, a common offset can occur for
   * each channel. To accomodate for different types of readout, this class
   * will apply first row wise and than column wise corrections.
   *
   * For row wise common mode correction, the class allows to specify the
   * number of rows which should used together as well es the number of
   * divisions per row. Same for column wise corrections
   */
  class CommonMode {
    public:
      /** Constructor
       * @param nRows number of rows for one row wise correction
       * @param nCols number of columns for one column wise correction
       * @param divRows number of divisions per row wise correction
       * @param divCols number of divisions per column wise correction
       */
      CommonMode(int nRows=1, int nCols=1, int divRows=1, int divCols=1):
        m_nRows(nRows), m_nCols(nCols), m_divRows(divRows), m_divCols(divCols),
        m_mask(0), m_noise(0), m_cutvalue(0) {};
      /** Apply common mode corrections to raw data */
      void apply(ADCValues& data);
      /** Return the calculated column wise corrections */
      const std::vector<double>& getCommonModesRow() const { return m_commonModeRow; }
      /** Return the calculated row wise corrections */
      const std::vector<double>& getCommonModesCol() const { return m_commonModeCol; }

      void setMask(const PixelMask* mask){
        m_mask = mask;
      }
      void setNoise(double cutvalue, const PixelNoise* noise){
        m_cutvalue = cutvalue;
        m_noise = noise;
      }
    protected:
      /** Calculate and apply common mode correction for a part of the matrix */
      double calculate(ADCValues &data, int startCol, int startRow, int nCols, int nRows);

      /** Number of rows per row wise correction */
      int m_nRows;
      /** Number of columns per column wise correction */
      int m_nCols;
      /** Number of divisions per row wise correction */
      int m_divRows;
      /** Number of divisions per column wise correction */
      int m_divCols;
      /** Caluclated row wise corrections */
      std::vector<double> m_commonModeRow;
      /** Caluclated column wise corrections */
      std::vector<double> m_commonModeCol;
      /** Matrix containing masked pixels */
      const PixelMask *m_mask;
      /** Matrix containing the pixel noise */
      const PixelNoise *m_noise;
      /** Cut value for ignoring high signal values during common mode correction */
      double m_cutvalue;
  };

  inline double CommonMode::calculate(ADCValues &data, int startCol, int startRow, int nCols, int nRows){
    static std::vector<double> pixelValues;
    pixelValues.clear();

    //Collect pixel data
    for(int x=startCol; x<startCol+nCols; ++x){
      for(int y=startRow; y<startRow+nRows; ++y){
        if(m_mask && (*m_mask)(x,y)!=0) {
          data(x,y) = 0;
          continue;
        }
        if(m_noise && data(x,y)>m_cutvalue*(*m_noise)(x,y)) continue;
        pixelValues.push_back(data(x,y));
      }
    }

    //Calculate median of selected pixel data
    if(pixelValues.empty()) return 0;
    std::vector<double>::iterator middle = pixelValues.begin() + (pixelValues.size())/2;
    std::nth_element(pixelValues.begin(), middle, pixelValues.end());

    //Apply correction to data
    for(int x=startCol; x<startCol+nCols; ++x){
      for(int y=startRow; y<startRow+nRows; ++y){
        if(m_mask && (*m_mask)(x,y)!=0) data(x,y) = 0;
        else data(x,y) -= *middle;
      }
    }

    //Return applied common mode correction
    return *middle;
  }

  inline void CommonMode::apply(ADCValues& data){
    int nCols(0), nRows(0);
    if(m_nRows>0) {
      m_commonModeRow.resize(data.getSizeY()/m_nRows*m_divRows);
    }else{
      m_commonModeRow.clear();
    }
    if(m_nCols>0){
      m_commonModeCol.resize(data.getSizeX()/m_nCols*m_divCols);
    }else{
      m_commonModeCol.clear();
    }

    //Calculate and apply row wise common mode correction
    nCols = data.getSizeX()/m_divRows;
    nRows = m_nRows;
    for(size_t i=0; i<m_commonModeRow.size(); ++i){
      int startCol = (i % m_divRows) * nCols;
      int startRow = (i / m_divRows) * nRows;
      m_commonModeRow[i] = calculate(data,startCol,startRow,nCols,nRows);
    }

    //Calculate and apply column wise common mode correction
    nCols = m_nCols;
    nRows = data.getSizeY()/m_divCols;
    for(size_t i=0; i<m_commonModeCol.size(); ++i){
      int startCol = (i / m_divCols) * nCols;
      int startRow = (i % m_divCols) * nRows;
      m_commonModeCol[i] = calculate(data,startCol,startRow,nCols,nRows);
    }
  }
}
#endif
