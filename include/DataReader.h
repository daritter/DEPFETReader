#ifndef DEPFET_DATAREADER_H
#define DEPFET_DATAREADER_H

#include <DEPFETReader/RawData.h>
#include <DEPFETReader/Event.h>

#include <fstream>
#include <map>

namespace DEPFET {
  class DataReader {
    public:
      DataReader():m_eventNumber(0),m_nEvents(-1),m_rawData(m_file),m_event(1),m_fold(2), m_useDCDBMapping(false) {}

      void open(const std::vector<std::string>& filenames, int nEvents=-1);
      bool skip(int nEvents);
      bool next(bool skip=false);
      Event& getEvent() { return m_event; }

      void setReadoutFold(int fold) { m_fold = fold; }
      void setUseDCDBMapping(bool useDCDBmapping) { m_useDCDBMapping=useDCDBmapping; }
    protected:
      bool openFile();
      bool readHeader();
      void readEvent(int dataSize);
      void convertData(RawData &rawdata, ADCValues &adcvalues);

      int m_eventNumber;
      int m_nEvents;
      int m_fold;
      bool m_useDCDBMapping;
      std::vector<std::string> m_filenames;
      std::ifstream m_file;
      RawData m_rawData;
      Event m_event;
  };
}

#endif
