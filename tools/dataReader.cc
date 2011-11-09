#include <DEPFETReader/RawData.h>
#include <DEPFETReader/DataReader.h>

#include <fstream>
#include <iostream>
#include <cmath>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <cstdlib>

using namespace std;

int main(int argc, char* argv[])
{
  /*int nEvents = std::atoi(argv[1]);
  ifstream dataFile;
  DEPFET::RawData data(dataFile);

  dataFile.exceptions(ifstream::failbit | ifstream::badbit);
  dataFile.open(argv[2], ios::in | ios::binary);

  for(int event=0; event<nEvents; ++event){
    data.readHeader();
    string indent = "  ";
    if(data.getDeviceType() == DEPFET::DEVICETYPE_GROUP) indent="";
    if(data.getDeviceType() == DEPFET::DEVICETYPE_INFO) indent="> ";
    cout << indent << "DEPFET::FileReader Header:" << std::endl;
    cout << indent << "  --> DeviceType: 0x" << hex << data.getDeviceType() << dec << std::endl;
    cout << indent << "  --> EventSize:  " << data.getEventSize() << std::endl;
    cout << indent << "  --> TriggerNr:  " << data.getTriggerNumber() << std::endl;
    cout << indent << "  --> ModuleNo:   " << data.getModuleNo() << std::endl;
    cout << indent << "  --> EventType:  0x" << hex << data.getEventType() << dec << std::endl;
    if(data.getDeviceType() == DEPFET::DEVICETYPE_GROUP)  continue;
    data.skipData();
  }
  dataFile.close();*/

  {
  int nSkip = std::atoi(argv[1]);
  int nEvents = std::atoi(argv[2]);
  DEPFET::DataReader reader;
  vector<string> filenames;
  for(int i=3; i<argc; ++i){ filenames.push_back(argv[i]); }
  reader.open(filenames,nEvents);
  reader.skip(nSkip);
  while(reader.next()){
    DEPFET::Event &event = reader.getEvent();
    cout << "Event " << event.getRunNumber() << "/" << event.getEventNumber() << endl;
    BOOST_FOREACH(DEPFET::ADCValues &data, event){
      cout << "  Module " << data.getModuleNr() << ", Trigger " << data.getTriggerNr() << ", "
        << data.getSizeX() << "x" << data.getSizeY() << endl;
    }
  }
  }
}
