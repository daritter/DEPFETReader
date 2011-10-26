#include <DEPFETReader/FileReader.h>

using namespace std;

namespace DEPFET {
  FileReader::NullStream FileReader::NO_LOGGING;

  FileReader::FileReader(const string& filename, ostream& logging): m_logging(logging)
  {
    m_logging << "DEPFET::FileReader: FileName=" << filename << std::endl;
    m_file.exceptions(ifstream::failbit | ifstream::badbit);
    m_file.open(filename.c_str(), ios::in | ios::binary);
  }

  int FileReader::readHeader()
  {
    while (true) {
      try {
        m_file.read((char*)&header, sizeof(header));
      } catch (ifstream::failure e) {
        m_logging << "DEPFET::FileReader: Error reading header: " << e.what() << endl;
        return 1;
      }
      m_logging << "DEPFET::FileReader Header:" << std::endl;
      m_logging << "  --> DeviceType: 0x" << hex << header.DeviceType << dec << std::endl;
      m_logging << "  --> EventSize:  " << header.EventSize << std::endl;
      m_logging << "  --> TriggerNr:  " << header.Triggernumber << std::endl;
      m_logging << "  --> ModuleNo:   " << header.ModuleNo << std::endl;
      m_logging << "  --> EventType:  0x" << hex << header.EventType << dec << std::endl;

      if (header.DeviceType == DEVICETYPE_INFO) {
        m_logging << "DEPFET::FileReader: Run Number=" << header.Triggernumber << std::endl;
        m_runNumber =  header.Triggernumber;
      }

      switch (header.DeviceType) {
        case DEVICETYPE_BAT:
        case DEVICETYPE_TPLL:
        case DEVICETYPE_UNKNOWN:
        case DEVICETYPE_TLU:
        case DEVICETYPE_INFO:
          m_logging << "DEPFET::FileReader Skip device 0x" << hex << header.DeviceType << ", ModuleNo=" << header.ModuleNo << std::endl;
          skipEvent();
          continue;
        default:
          return 0;
      }
    }
  }

  int FileReader::readGroupHeader()
  {
    int nModules(0);
    readHeader();
    if (header.EventType == EVENTTYPE_RUN_BEGIN) {
      nModules = (int)((header.EventSize - 2) / 2);
      m_logging << "DEPFET::FileReader Group Header: nModules=" << nModules << std::endl;
    } else if (header.EventType == EVENTTYPE_DATA) {
      return -1;
    }
    return nModules;
  }

  int FileReader::readEventHeader()
  {
    if (readHeader() > 0) return 1;
    switch (header.DeviceType) {
      case DEVICETYPE_GROUP:
        return 100;
      case DEVICETYPE_DEPFET:
      case DEVICETYPE_DEPFET_128:
      case DEVICETYPE_DEPFET_DCD:
        switch (header.EventType) {
          case EVENTTYPE_RUN_BEGIN:
            m_logging << "DEPFET::FileReader Module=" << header.ModuleNo << " Begin of Run" << std::endl;
            return 0;
          case EVENTTYPE_RUN_END:
            m_logging << "DEPFET::FileReader Module=" << header.ModuleNo << " End of Run" << std::endl;
            return 1;
          case EVENTTYPE_DATA:
            return 2;
        }
        break;
      case DEVICETYPE_TLU:
        if (header.EventType == EVENTTYPE_DATA) {
          skipEvent();
          return 3;
        }
        break;
      case DEVICETYPE_INFO:
        skipEvent();
        return 4;
        break;
    }
    return 0;
  }

  void FileReader::skipWords(int n)
  {
    m_file.seekg(n, ios::cur);
  }

  void FileReader::skipEvent()
  {
    skipWords((header.EventSize - 2)*4);
  }

  int FileReader::readEvent(RawData& rawdata)
  {
    unsigned int startgate;
    struct InfoWord *infoWord;
    try {
      m_file.read((char*)&startgate, sizeof(startgate));
    } catch (ifstream::failure e) {


      m_logging << "DEPFET::FileReader EOF" << std::endl;
      return 1;
    }
    infoWord = (InfoWord*) & startgate;
    m_logging << "InfoWord.framecnt      = 0x" << hex << infoWord->framecnt << endl;
    m_logging << "InfoWord.startgate     = 0x" << hex << infoWord->startgate << endl;
    m_logging << "InfoWord.zerosupp      = 0x" << hex << infoWord->zerosupp << endl;
    m_logging << "InfoWord.startgate_ver = 0x" << hex << infoWord->startgate_ver << endl;
    m_logging << "InfoWord.temperature   = 0x" << hex << infoWord->temperature << endl << dec;
    int eventsize = header.EventSize - 3;
    m_logging << "DEPFET::FileReader Eventsize=" << eventsize << endl;
    rawdata.read(m_file, eventsize, header.DeviceType, (startgate >> 10) & 0x7f);
    return 0;
  }

} //namespace DEPFET
