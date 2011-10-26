#ifndef DEPFET_DATAREADER_H
#define DEPFET_DATAREADER_H

#include <DEPFETReader/FileReader.h>
#include <DEPFETReader/Event.h>
#include <DEPFETReader/S3AConverter.h>
#include <DEPFETReader/S3BConverter.h>
#include <DEPFETReader/DCDConverter.h>

#include <map>

namespace DEPFET {
  class DataReader {
    public:
      DataReader():m_eventNumber(0),m_nEvents(-1),m_file(0),m_event(0) {}
      ~DataReader() {
        if(m_file) delete m_file;
      }

      int open(const std::string& filename, int yearflag=0, int nEvents=-1);
      bool skip(int nEvents);
      bool next();
      Event& getEvent() { return m_event; }

      void setReadoutFold(int fold) { m_fold = fold; }
      void setUseDCDBMapping(bool useDCDBmapping) { m_useDCDBMapping=useDCDBmapping; }
    protected:
      void convertData(RawData &rawdata, ADCValues &adcvalues);
      int m_eventNumber;
      int m_nEvents;
      int m_fold;
      bool m_useDCDBMapping;
      FileReader* m_file;
      Event m_event;
      RawData m_rawData;
      std::map<int,int> m_mod2Index;
  };
}

#endif
