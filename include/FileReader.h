#ifndef DEPFET_FILEREADER_H
#define DEPFET_FILEREADER_H

#include <DEPFETReader/RawData.h>

#include <iostream>
#include <fstream>
#include <string>

namespace DEPFET {

#define MAXDATA 10000   //-- in words; 8195 * 4 = 32780 bytes -> size of one depfet
#define MAXMOD  6

  struct Header {
    unsigned int    EventSize: 20;
    unsigned short  flag0: 1;
    unsigned short  flag1: 1;
    unsigned short  EventType: 2;
    unsigned short  ModuleNo:  4;
    unsigned short  DeviceType: 4;
    unsigned int    Triggernumber;
  } ;

  struct InfoWord  {
    unsigned int framecnt: 10; // number of Bits
    unsigned int startgate: 10; //jf new for 128x128
    unsigned int zerosupp: 1;
    unsigned int startgate_ver: 1;
    unsigned int temperature: 10;
  };

  struct DepfetEvent {
    unsigned int HEADER;
    unsigned int Trigger;
    unsigned int startgate;
    unsigned int data[MAXDATA];
  };

  //! DEPFET File reader
  /*! A class to read and decode a DEPFET file
   *
   *  @author Yulia Furletova, Uni-Bonn <mailto:yulia@mail.cern.ch>
   *  @version $Id: DEPFET_FR.h,v 1.2 2008/11/13 10:43:56 bulgheroni Exp $
   */


  //Null stream as default output to make it quiet

  class FileReader {

  private:
    std::ifstream m_file;
    std::ostream& m_logging;

  public:
    static struct NullStream: std::ostream {
      NullStream(): std::ios(0), std::ostream(0) {}
    } NO_LOGGING;

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

    enum { NUM_COL = 64, NUM_ROW = 128 };

    struct InfoWord infoword;
    struct Header header;
    struct InfoWord *infoword1;

    int m_runNumber;

    FileReader(const std::string &filename, std::ostream& logging = NO_LOGGING);
    ~FileReader() {};


    void skipWords(int n);
    void skipEvent();

    int readHeader();
    int readGroupHeader();
    int readEventHeader();
    int readEvent(RawData &rawdata);
  };
}
#endif /*DEPFET_FILEREADER_H*/
