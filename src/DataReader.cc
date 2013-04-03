#include <DEPFETReader/DataReader.h>
#include <DEPFETReader/S3AConverter.h>
#include <DEPFETReader/S3BConverter.h>
#include <DEPFETReader/DCDConverter.h>
#include <algorithm>
#include <iostream>

namespace DEPFET {

  void DataReader::open(const std::vector<std::string>& filenames, int nEvents)
  {
    //Close open files
    m_file.close();
    m_file.clear();

    //Set number of events
    m_nEvents = nEvents;
    m_eventNumber = 0;

    //Set list of filenames to read in succession
    m_filenames = filenames;
    std::reverse(m_filenames.begin(), m_filenames.end());
    openFile();
  }

  bool DataReader::openFile()
  {
    if (m_filenames.empty()) return false;
    //Open the next file from the stack of files
    std::string filename = m_filenames.back();
    //std::cout << "Opening " << filename << std::endl;
    m_file.close();
    m_file.clear();
    m_file.open(filename.c_str(), std::ios::in | std::ios::binary);
    if (!m_file) {
      throw std::runtime_error("Error opening file " + filename);
    }
    return true;
  }

  bool DataReader::readHeader()
  {
    //Read one header from file. If an error occured, try the next file
    while (true) {
      m_rawData.readHeader();
      //No error, so return true
      if (!m_file.fail()) return true;

      //We have an error, check if there is an additional file to open
      m_filenames.pop_back();
      if (!openFile()) return false;
    }
  }

  bool DataReader::skip(int nEvents)
  {
    m_event.clear();
    for (int i = 0; i < nEvents; ++i) {
      if (!next(true)) return false;
    }
    return true;
  }

  bool DataReader::next(bool skip)
  {
    if (!skip) {
      m_event.clear();
      ++m_eventNumber;
      //check if max number of events is reached
      if (m_nEvents > 0 && m_eventNumber > m_nEvents) return false;
    }

    while (readHeader()) {
      if (m_rawData.getDeviceType() == DEVICETYPE_INFO) {
        m_event.setRunNumber(m_rawData.getTriggerNr());
        continue;
      }
      if (m_rawData.getDeviceType() == DEVICETYPE_GROUP) {
        if (m_rawData.getEventType() == EVENTTYPE_DATA) {
          //If we are in skipping mode we don't read the data
          if (skip) {
            m_rawData.skipData();
            return true;
          }

          //Read event data
          m_event.setEventNumber(m_rawData.getTriggerNr());
          readEvent(m_rawData.getEventSize() - 2);
          return true;
        }
      }
      //Skip all other headers
      m_rawData.skipData();
    }
    return false;
  }

  void DataReader::readEvent(int dataSize)
  {
    size_t index(0);
    while (dataSize > 0) {
      if (!readHeader()) {
        throw std::runtime_error("Problem reading event from file");
      }
      if (m_rawData.getEventType() != EVENTTYPE_DATA) {
        throw std::runtime_error("Expected data event, got something else");
      }
      dataSize -= m_rawData.getEventSize();
      if (dataSize < 0) {
        throw std::runtime_error("Eventsize does not fit into remaining data size");
      }

      //Read data
      m_rawData.readData();
      size_t alreadyUsed = 0;
      int frameNr = 0;
      while (alreadyUsed < m_rawData.getDataSize()) {
        m_event.resize(index + 1);
        ADCValues& adcvalues = m_event.at(index++);
        adcvalues.setModuleNr(m_rawData.getModuleNr());
        adcvalues.setTriggerNr(m_rawData.getTriggerNr());
        adcvalues.setStartGate(m_rawData.getStartGate());
        adcvalues.setFrameNr(frameNr++);
        alreadyUsed += convertData(m_rawData, adcvalues);
        m_rawData.setOffset(alreadyUsed);
      }
    }
  }


  size_t DataReader::convertData(RawData& rawdata, ADCValues& adcvalues)
  {
    switch (rawdata.getDeviceType()) {
      case DEVICETYPE_DEPFET_128: //S3B
        if (m_fold == 4) {
          S3BConverter4Fold convert;
          return convert(rawdata, adcvalues);
        } else {
          S3BConverter2Fold convert;
          return convert(rawdata, adcvalues);
        }
        break;
      case DEVICETYPE_DEPFET_DCD: //DCD
        if (m_fold == 4) {
          DCDConverter4Fold convert(m_useDCDBMapping);
          return convert(rawdata, adcvalues);
        } else {
          DCDConverter2Fold convert(m_useDCDBMapping);
          return convert(rawdata, adcvalues);
        }
        break;
      default: //S3A/
        S3AConverter convert;
        return convert(rawdata, adcvalues);
    }
  }
}
