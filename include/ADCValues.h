#ifndef DEPFET_ADCVALUES_H
#define DEPFET_ADCVALUES_H

#include <stdexcept>

namespace DEPFET {
  /** Class to represent a matrix of values.
   * Offers flat or 2D access to the values and to add and substract other matrices */
  template<class T=double> class ValueMatrix {
  public:
    /** Datatype for the matrix */
    typedef typename std::vector<T>::value_type value_type;
    /** Construct an empty matrix with no elements and zero size */
    ValueMatrix(): m_sizeX(0), m_sizeY(0) {}
    /** Construct a matrix with a given size */
    ValueMatrix(size_t sizeX, size_t sizeY): m_sizeX(sizeX), m_sizeY(sizeY), m_data(sizeX* sizeY) {}

    /** resize the matrix to the given dimensions */
    void setSize(size_t sizeX, size_t sizeY) { m_sizeX = sizeX; m_sizeY = sizeY; clear(); }
    /** resize the matrix to match the size of another matrix */
    template<class T2> void setSize(const ValueMatrix<T2>& other) { setSize(other.getSizeX(), other.getSizeY()); }
    /** clear all elements */
    void clear() { m_data.clear(); m_data.resize(m_sizeX * m_sizeY); }

    /** get size in x */
    size_t getSizeX() const { return m_sizeX; }
    /** get size in y */
    size_t getSizeY() const { return m_sizeY; }
    /** get total number of elements */
    size_t getSize() const { return m_data.size(); }
    /** check if the matrix has a nonzero size */
    bool operator!() const { return m_data.empty(); }

    /** return value of a given position, no boundary check */
    value_type operator()(size_t x, size_t y) const { return m_data[x * m_sizeY + y]; }
    /** return value of a given position with boundary check */
    value_type at(size_t x, size_t y) const {
      if (0 > x || x >= m_sizeX || 0 > y || y >= m_sizeY)
        throw std::runtime_error("index out of bounds");
      return m_data[x * m_sizeY + y];
    }
    /** return value of an element of the flat array, no boundary check */
    value_type operator[](size_t index) const { return m_data[index]; }

    /** return reference to a given position, no boundary check */
    value_type& operator()(size_t x, size_t y) { return m_data[x * m_sizeY + y]; }
    /** return reference to a given position with boundary check */
    value_type& operator[](size_t index) { return m_data[index]; }
    /** return reference to an element of the flat array, no boundary check */
    value_type& at(size_t x, size_t y) {
      if (0 > x || x >= m_sizeX || 0 > y || y >= m_sizeY)
        throw std::runtime_error("index out of bounds");
      return m_data[x * m_sizeY + y];
    }

    /** substract another matrix */
    template<class T2> void substract(const ValueMatrix<T2>& other, double scale = 1) {
      if (other.getSizeX() != m_sizeX || other.getSizeY() != m_sizeY) {
        throw std::runtime_error("Dimensions do not match");
      }
      for (size_t i = 0; i < m_data.size(); ++i) m_data[i] -= scale * (value_type) other[i];
    }

    /** add another matrix */
    template<class T2> void add(const ValueMatrix<T2>& other, double scale = 1) {
      if (other.getSizeX() != m_sizeX || other.getSizeY() != m_sizeY) {
        throw std::runtime_error("Dimensions do not match");
      }
      for (size_t i = 0; i < m_data.size(); ++i) m_data[i] += scale * (value_type) other[i];
    }

    /** set matrix from given matrix */
    template<class T2> void set(const ValueMatrix<T2>& other, double scale = 1) {
      if (other.getSizeX() != m_sizeX || other.getSizeY() != m_sizeY) {
        throw std::runtime_error("Dimensions do not match");
      }
      for (size_t i = 0; i < m_data.size(); ++i) m_data[i] = scale * (value_type) other[i];
    }

  protected:
    /** size in X */
    size_t m_sizeX;
    /** size in Y */
    size_t m_sizeY;
    /** vector containing the data */
    std::vector<T> m_data;
  };

  /** Typedef used for a Pixel Mask. Normally we would use bool but
   * std::vector<bool> ist specialized so unsigned char is more suitable */
  typedef ValueMatrix<unsigned char> PixelMask;
  /** Typedef used for the pixel noise map */
  typedef ValueMatrix<double> PixelNoise;

  /** Class for adc values from a matrix, including some additional information */
  class ADCValues: public ValueMatrix<double> {
  public:
    /** default constructor */
    ADCValues(): ValueMatrix<double>(), m_moduleNr(0), m_triggerNr(0), m_startGate(-1) {}
    /** get the number of the module */
    int getModuleNr() const { return m_moduleNr; }
    /** get the trigger number */
    int getTriggerNr() const { return m_triggerNr; }
    /** get the start gate where readout starts */
    int getStartGate() const { return m_startGate; }
    /** get the frame number. 0 for the normal frame, 1..n for trailing frames */
    int getFrameNr() const { return m_frameNr; }
    /** set the number of the module */
    void setModuleNr(int moduleNr) { m_moduleNr = moduleNr; }
    /** set the trigger number */
    void setTriggerNr(int triggerNr) { m_triggerNr = triggerNr; }
    /** set the start gate where readout starts */
    void setStartGate(int startGate) { m_startGate = startGate; }
    /** set the frame number */
    void setFrameNr(int frameNr) { m_frameNr = frameNr; }
  protected:
    /** module number */
    int m_moduleNr;
    /** trigger number */
    int m_triggerNr;
    /** start gate */
    int m_startGate;
    /** frame number,  0 for the normal frame, 1..n for trailing frames */
    int m_frameNr;
  };

}

#endif
