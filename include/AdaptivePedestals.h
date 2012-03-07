/**************************************************************************
 * BASF2 (Belle Analysis Framework 2)                                     *
 * Copyright(C) 2010 - Belle II Collaboration                             *
 *                                                                        *
 * Author: The Belle II Collaboration                                     *
 * Contributors: Martin Ritter                                            *
 *                                                                        *
 * This software is provided "as is" without any warranty.                *
 **************************************************************************/

#ifndef ADAPTIVEPEDESTALS_H
#define ADAPTIVEPEDESTALS_H

#include <vector>
#include <cmath>
#include <cstring>
#include <iostream>

namespace DEPFET {
  template<class T> class RingBuffer {
  public:
    typedef T value_type;
    RingBuffer(size_t size): m_pos(0), m_size(size) {
      m_buffer.reserve(size);
    }
    size_t capacity() const { return m_size; }
    size_t size() const { return m_buffer.size(); }
    size_t pos() const { return m_pos; }
    T& operator[](size_t i) { return m_buffer[(m_pos + i) % m_size]; }
    T operator[](size_t i) const { return m_buffer[(m_pos + i) % m_size]; }
    void add(const T& element) {
      if (size() < capacity()) {
        m_buffer.push_back(element);
      } else {
        m_buffer[(m_pos++ + m_size) % m_size] = element;
        if (m_pos >= m_size) m_pos %= m_size;
      }
    }
  protected:
    size_t m_pos;
    size_t m_size;
    std::vector<T> m_buffer;
  };


  class AdaptivePedestal {
  public:
    enum { NSIGMA = 4 };
    AdaptivePedestal(int interval = 100, int events = 200):
      m_mean(0), m_sigma(1e5), m_interval(interval), m_pos(0), m_buffer(events) {}
    operator double() {
      return getMean();
    }
    double getMean() const { return m_mean; }
    double getSigma() const { return m_sigma; }

    void addRaw(int adu) {
      m_buffer.add(adu);
      if (++m_pos > m_interval) calculate();
    }

    void calculate() {
      m_pos = 0;
      calculateMeanSigma(m_mean, m_sigma);
      if (NSIGMA > 0) calculateMeanSigma(m_mean, m_sigma, NSIGMA * m_sigma);
    }

    void setMean(double mean, double sigma = 1e5) { m_mean = mean; m_sigma = sigma; }

  protected:
    void calculateMeanSigma(double& mean, double& sigma, double cutValue = 0) {
      int entries(0);
      double newMean(0), variance(0);
      for (size_t i = 0; i < m_buffer.size(); ++i) {
        const int x = m_buffer[i];
        if (cutValue > 0 && std::fabs(x - mean) > cutValue) continue;
        entries++;
        const double oldMean = newMean;
        newMean += (x - oldMean) / entries;
        variance += (x - oldMean) * (x - newMean);
      }
      mean = newMean;
      sigma = std::sqrt(variance / entries);
    }

    double m_mean;
    double m_sigma;
    int m_interval;
    int m_pos;
    RingBuffer<int> m_buffer;
  };


} //DEPFET namespace
#endif
