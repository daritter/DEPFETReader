#include <DEPFETReader/DataReader.h>

namespace DEPFET {

  int DataReader::open(const std::string& filename, int yearflag, int nEvents)
  {
    //Set number of events
    m_nEvents = nEvents;
    m_eventNumber = 0;

    if(m_file) delete m_file;
    m_file = new FileReader(filename);

    int nModules = 1;
    if(yearflag != 9999){
      nModules = m_file->readGroupHeader();
      if(nModules<0) throw std::runtime_error("Could not read header");
    }
    //override number of modules
    if(yearflag==6) nModules=5;

    //Make sure event data is present
    m_event.resize(nModules);

    //Set ModuleNr for event and mapping of moduleNo->index
    if(yearflag!=9999){
      m_mod2Index.clear();
      for(int i=0; i<nModules; ++i){
        m_file->readHeader();
        m_event.at(i).setModuleNr(m_file->header.ModuleNo);
        m_mod2Index[m_file->header.ModuleNo] = i;
      }
    }

    return m_file->m_runNumber;
  }

  bool DataReader::skip(int nEvents)
  {
    m_event.clear();
    for(int i=0; i<nEvents; ++i){
      int rc = m_file->readEventHeader();
      //This should be the start of an event
      if(rc!=100) return false;
      m_file->skipEvent();
    }
    return true;
  }

  bool DataReader::next()
  {
    m_event.clear();
    ++m_eventNumber;
    //check if max events is reached
    if(m_nEvents>0 && m_eventNumber>m_nEvents) return false;

    int rc = m_file->readEventHeader();
    //This should be the start of an event
    if(rc!=100) return false;

    m_event.setRunNumber(m_file->m_runNumber);
    m_event.setEventNumber(m_file->header.Triggernumber);

    for(size_t i=0; i<m_mod2Index.size(); ++i){
      if(m_file->readEventHeader()!=2) return false;
      m_file->readEvent(m_rawData);
      int index = m_mod2Index[m_file->header.ModuleNo];
      ADCValues& adcvalues = m_event.at(index);
      //Now convert the data
      convertData(m_rawData,adcvalues);
    }
    return true;
  }

  void DataReader::convertData(RawData &rawdata, ADCValues &adcvalues)
  {
    switch(rawdata.getDeviceType()){
      case FileReader::DEVICETYPE_DEPFET_128: //S3B
        if(m_fold==4){
          S3BConverter4Fold convert;
          convert(rawdata,adcvalues);
        }else{
          S3BConverter2Fold convert;
          convert(rawdata,adcvalues);
        }
        break;
      case FileReader::DEVICETYPE_DEPFET_DCD: //DCD
        if(m_fold==4){
          DCDConverter4Fold convert(m_useDCDBMapping);
          convert(rawdata,adcvalues);
        }else{
          DCDConverter2Fold convert(m_useDCDBMapping);
          convert(rawdata,adcvalues);
        }
        break;
      default: //S3A/
        S3AConverter convert;
        convert(rawdata,adcvalues);
    }
  }
}
