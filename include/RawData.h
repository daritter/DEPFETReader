#ifndef DEPFET_RAWDATA_H
#define DEPFET_RAWDATA_H

#include <vector>
#include <istream>
#include <DEPFETReader/DataView.h>

namespace DEPFET {

  enum DeviceTypes {
    DEVICETYPE_GROUP      = 0x0,
    DEVICETYPE_DEPFET     = 0x2,
    DEVICETYPE_DEPFET_128 = 0x3,
    DEVICETYPE_DEPFET_DCD = 0x4,
    DEVICETYPE_BAT        = 0x5,
    DEVICETYPE_TPLL       = 0xA,
    DEVICETYPE_UNKNOWN    = 0xB,
    DEVICETYPE_TLU        = 0xD,   //---  new TLU  event
    DEVICETYPE_INFO       = 0xE,   //---  new info header for run number + etc
    DEVICETYPE_OTHER      = 0xF
  };

  enum EventTypes {
    EVENTTYPE_RUN_BEGIN   = 0x0,
    EVENTTYPE_RUN_END     = 0x1,
    EVENTTYPE_DATA        = 0x2,
  };

  class RawData {
    public:
      /** Struct containing the header information for each record */
      struct Header {
        unsigned int    eventSize: 20;
        unsigned short  flag0: 1;
        unsigned short  flag1: 1;
        unsigned short  eventType: 2;
        unsigned short  moduleNo:  4;
        unsigned short  deviceType: 4;
        unsigned int    triggerNumber;
      };

      /** Struct containing the frame information for each data blob */
      struct InfoWord  {
        unsigned int framecnt: 10; // number of Bits
        unsigned int startGate: 7; //jf new for 128x128
        unsigned int padding: 3;
        unsigned int zerosupp: 1;
        unsigned int startgate_ver: 1;
        unsigned int temperature: 10;
      };

      /** Constructor taking a reference to the stream from which to read the data */
      RawData(std::istream& stream):m_stream(stream){}

      /** Return a view of the data */
      template<class T> DataView<T> getView(size_t nX=0, size_t nY=0) const {
        return DataView<T>(&m_data.front(), m_data.size(), nX, nY);
      };

      /** Read the next Header record */
      void readHeader(){
        m_data.clear();
        m_stream.read((char*)&m_header, sizeof(m_header));
      }

      /** Read the next data blob */
      void readData() {
        int dataSize = m_header.eventSize-3;
        m_data.resize(dataSize);
        m_stream.read((char*)&m_infoWord, sizeof(m_infoWord));
        m_stream.read((char*)&m_data.front(), sizeof(unsigned int)*dataSize);
      }

      /** Skip the next data blob */
      void skipData() {
        m_stream.seekg((m_header.eventSize - 2)*sizeof(unsigned int), std::ios::cur);
      }

      /** Return the Event Type */
      int getEventType() const { return m_header.eventType; }
      /** Return the Trigger Number */
      int getTriggerNr() const { return m_header.triggerNumber; }
      /** Return the Module Number */
      int getModuleNr() const { return m_header.moduleNo; }
      /** Return the Device Type */
      int getDeviceType() const { return m_header.deviceType; }
      /** Return the Event size (including header) */
      int getEventSize() const { return m_header.eventSize; }
      /** Return the data size after reading the data blob */
      int getDataSize() const { return m_data.size(); }
      /** Return the start gate of the readout frame */
      int getStartGate() const { return m_infoWord.startGate; }
      /** Return the temperature value */
      float getTemperature() const { return m_infoWord.temperature/4.0; }
    protected:
      /** Reference to the stream of data */
      std::istream& m_stream;
      /** Struct containing the header information */
      Header m_header;
      /** Struct containing the info word at the begin of each data blob */
      InfoWord m_infoWord;
      /** Array containing the raw data */
      std::vector<unsigned int> m_data;
  };

}
#endif
