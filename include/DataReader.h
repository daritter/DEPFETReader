#ifndef DEPFET_DATAREADER_H
#define DEPFET_DATAREADER_H

#include <DEPFETReader/RawData.h>
#include <DEPFETReader/Event.h>

#include <fstream>
#include <map>

namespace DEPFET {
  /** Class to read binary DEPFET file and return the raw adc values for each readout event */
  class DataReader {
  public:
    /** constructor to create a new instance */
    DataReader(): m_eventNumber(0), m_nEvents(-1), m_fold(2), m_useDCDBMapping(true), m_rawData(m_file), m_event(1) {}

    /** open a list of files and limit the readout to nEvents */
    void open(const std::vector<std::string>& filenames, int nEvents = -1);
    /** skip a given number of events from the data file */
    bool skip(int nEvents);
    /** read next event, if skip=true the event data is not updated.
     * Returns false if at end of file or maximum number of events is reached */
    bool next(bool skip = false);
    /** return reference to the event data */
    Event& getEvent() { return m_event; }

    /** set the readout fold, has to be known to configure the binary format
     * interpreter, normally 2 fold (curo readout) or 4fold (dcd readout) */
    void setReadoutFold(int fold) { m_fold = fold; }
    /** configure if DCDB mapping should be used, only relevant for dcd readout */
    void setUseDCDBMapping(bool useDCDBmapping) { m_useDCDBMapping = useDCDBmapping; }
  protected:
    /** actually open the next file */
    bool openFile();
    /** read the next event header */
    bool readHeader();
    /** read the next event */
    void readEvent(int dataSize);
    /** convert the raw binary data to ADCValues */
    size_t convertData(RawData& rawdata, ADCValues& adcvalues);

    /** current event number */
    int m_eventNumber;
    /** maximal number of events to read */
    int m_nEvents;
    /** configured readout fold */
    int m_fold;
    /** use dcdb mapping? */
    bool m_useDCDBMapping;
    /** list of filenames */
    std::vector<std::string> m_filenames;
    /** currently open file */
    std::ifstream m_file;
    /** rawdata structure used for reading the binary blobs */
    RawData m_rawData;
    /** event structure to fill the data in */
    Event m_event;
  };
}

#endif
