#ifndef DEPFET_EVENT_H
#define DEPFET_EVENT_H

#include <DEPFETReader/ADCValues.h>
#include <vector>

namespace DEPFET {
  class Event: public std::vector<ADCValues> {
  public:
    Event(int nModules = 0): std::vector<ADCValues>(nModules), m_runNumber(0), m_eventNumber(0) {}

    int getRunNumber() const { return m_runNumber; }
    int getEventNumber() const { return m_eventNumber; }

    void setRunNumber(int runNumber) { m_runNumber = runNumber; }
    void setEventNumber(int eventNumber) { m_eventNumber = eventNumber; }

    void clear() {
      for (iterator it = begin(); it != end(); ++it) it->clear();
    }
  protected:
    int m_runNumber;
    int m_eventNumber;
  };
}

#endif
