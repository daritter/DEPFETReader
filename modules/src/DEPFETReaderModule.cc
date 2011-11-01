/**************************************************************************
 * BASF2 (Belle Analysis Framework 2)                                     *
 * Copyright(C) 2010-2011  Belle II Collaboration                         *
 *                                                                        *
 * Author: The Belle II Collaboration                                     *
 * Contributors: Martin Ritter                                            *
 *                                                                        *
 * This software is provided "as is" without any warranty.                *
 **************************************************************************/

#include <DEPFETReader/modules/DEPFETReaderModule.h>

#include <framework/gearbox/Unit.h>
#include <framework/logging/Logger.h>
#include <framework/datastore/DataStore.h>
#include <framework/datastore/StoreArray.h>
#include <framework/datastore/StoreObjPtr.h>
#include <framework/dataobjects/EventMetaData.h>
#include <pxd/dataobjects/PXDDigit.h>

#include <algorithm>
#include <boost/format.hpp>
#include <boost/foreach.hpp>

using namespace std;
using namespace Belle2;
using namespace DEPFET;

REG_MODULE(DEPFETReader)

DEPFETReaderModule::DEPFETReaderModule() : Module(), m_commonMode(2,1,2,1)
{
  //Set module properties
  setDescription("Read raw DEPFET data");
  setPropertyFlags(c_Input);

  //Parameter definition
  addParam("inputFiles", m_inputFiles, "Name of the data files");
  addParam("sigmaCut", m_sigmaCut, "Zero suppression cut to apply", 5.0);
  addParam("readoutFold", m_readoutFold, "Readout fold (2 or 4 usually)", 2);
  addParam("skipEvents", m_skipEvents, "Skip this number of events before starting.", 0);
  addParam("calibrationEvents", m_calibrationEvents, "Calibrate using this number of events before starting.", 1000);
}

void DEPFETReaderModule::progress(int event, int maxOrder){
  int order = (event == 0) ? 1 : static_cast<int>(std::min(std::log10(event), (double)maxOrder));
  int interval = static_cast<int>(std::pow(10., order));
  if (event % interval == 0) B2INFO("Events read: " << event);
}

void DEPFETReaderModule::calculatePedestals(){
  B2INFO("Calculating pedestals");
  int eventNr(1);
  double cutValue(0.0);
  while(m_reader.next()){
    const Event& event = m_reader.getEvent();
    const ADCValues& data = event[0];
    if(eventNr==1) m_pedestals.setSize(data);
    if(eventNr==20) cutValue = m_sigmaCut;
    for(size_t x=0; x<data.getSizeX(); ++x){
      for(size_t y=0; y<data.getSizeY(); ++y){
        m_pedestals(x,y).add(data(x,y),1.0,cutValue);
      }
    }
    progress(eventNr++,4);
  }
}

void DEPFETReaderModule::calculateNoise(){
  B2INFO("Calculating noise");
  ValueMatrix<IncrementalMean> noise;
  noise.setSize(m_pedestals);
  m_noise.setSize(m_pedestals);
  int eventNr(1);
  while(m_reader.next()){
    Event& event = m_reader.getEvent();
    ADCValues& data = event[0];
    data.substract(m_pedestals);
    m_commonMode.apply(data);
    for(size_t x=0; x<data.getSizeX(); ++x){
      for(size_t y=0; y<data.getSizeY(); ++y){
        double signal = data(x,y);
        if(std::fabs(signal)>m_sigmaCut*m_pedestals(x,y).getSigma()) continue;
        noise(x,y).add(signal);
      }
    }
    progress(eventNr++,4);
  }

  for(size_t x=0; x<noise.getSizeX(); ++x){
    for(size_t y=0; y<noise.getSizeY(); ++y){
      m_noise(x,y) = noise(x,y).getSigma();
    }
  }
}

void DEPFETReaderModule::initialize()
{
  //Initialize PXDDigits collection
  StoreArray<PXDDigit> PXDDigits;

  reverse(m_inputFiles.begin(),m_inputFiles.end());
  m_inputFile = m_inputFiles.back();
  m_inputFiles.pop_back();

  //caluclate pedestals
  m_reader.open(m_inputFile, 0, m_calibrationEvents);
  m_reader.skip(m_skipEvents);
  calculatePedestals();

  //caluclate noise
  m_reader.open(m_inputFile, 0, m_calibrationEvents);
  m_reader.skip(m_skipEvents);
  calculateNoise();

  //Open file again
  B2INFO("Opening file" << m_inputFile);
  m_reader.open(m_inputFile);
  m_reader.skip(m_skipEvents);
}


void DEPFETReaderModule::event()
{
  StoreArray<PXDDigit>   storeDigits;

  while(!m_reader.next()){
    if(m_inputFiles.empty()){
      StoreObjPtr <EventMetaData> eventMetaDataPtr;
      eventMetaDataPtr->setEndOfData();
      return;
    }
    m_inputFile = m_inputFiles.back();
    m_inputFiles.pop_back();
    B2INFO("Opening file" << m_inputFile);
    m_reader.open(m_inputFile,9999);
  }

  Event& event = m_reader.getEvent();
  ADCValues& data = event[0];
  data.substract(m_pedestals);
  m_commonMode.apply(data);
  for(size_t x=0; x<data.getSizeX(); ++x){
    for(size_t y=0; y<data.getSizeY(); ++y){
      double signal = data(x,y);
      if(signal<m_sigmaCut*m_noise(x,y)) continue;
      //Create new digit
      int digIndex = storeDigits->GetLast() + 1;
      new(storeDigits->AddrAt(digIndex)) PXDDigit(VxdID(1,1,1),x,y,0,0,max(0.0,signal));
    }
  }
}
