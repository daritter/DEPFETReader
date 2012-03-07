#ifndef DEPFET_INCREMENTALMEAN_H
#define DEPFET_INCREMENTALMEAN_H

#include <cmath>

namespace DEPFET {

  class IncrementalMean {
  public:
    IncrementalMean(): m_entries(0), m_mean(0), m_variance(0) {}
    void clear() {
      m_entries = 0;
      m_mean = 0;
      m_variance = 0;
    }
    void add(double x, double weight = 1.0, double sigmaCut = 0.0) {
      if (sigmaCut > 0 && std::fabs(x - getMean()) > getSigma()*sigmaCut) return;
      m_entries += weight;
      const double oldMean = m_mean;
      m_mean += weight * (x - oldMean) / m_entries;
      m_variance += weight * (x - oldMean) * (x - m_mean);
    }
    double getEntries() const { return m_entries; }
    double getMean() const { return m_mean; }
    double getSigma() const { return std::sqrt(m_variance / m_entries); }
    operator double() const { return getMean(); }
    operator int() const { return round(getMean()); }
  protected:
    double m_entries;
    double m_mean;
    double m_variance;
  };

}
#endif
