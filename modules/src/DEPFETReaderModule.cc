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
#include <vxd/geometry/GeoCache.h>

#include <algorithm>
#include <boost/format.hpp>
#include <boost/foreach.hpp>

using namespace std;
using namespace Belle2;
using namespace DEPFET;

REG_MODULE(DEPFETReader)

DEPFETReaderModule::DEPFETReaderModule() : Module(), m_commonMode(2, 1, 2, 1), m_currentFrame(0)
{
  //Set module properties
  setDescription("Read raw DEPFET data");
  setPropertyFlags(c_Input);

  //Parameter definition
  addParam("inputFiles", m_inputFiles, "Name of the data files");
  addParam("sigmaCut", m_sigmaCut, "Zero suppression cut to apply", 5.0);
  addParam("readoutFold", m_readoutFold, "Readout fold (2 or 4 usually)", 2);
  addParam("isDCD", m_dcd, "0 for 2 half row common mode, 1 for 4 row common mode substraction", 0);
  addParam("skipEvents", m_skipEvents, "Skip this number of events before starting.", 0);
  addParam("trailingFrames", m_trailingFrames, "Number of trailing frames", 0);
  //addParam("calibrationEvents", m_calibrationEvents, "Calibrate using this number of events before starting.", 1000);
  addParam("calibrationFile", m_calibrationFile, "File to read calibration from");
}

void DEPFETReaderModule::progress(int event, int maxOrder)
{
  int order = (event == 0) ? 1 : static_cast<int>(std::min(std::log10(event), (double)maxOrder));
  int interval = static_cast<int>(std::pow(10., order));
  if (event % interval == 0) B2INFO("Events read: " << event);
}

void DEPFETReaderModule::calculatePedestals()
{
  /*B2INFO("Calculating pedestals");
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
  }*/
}

void DEPFETReaderModule::calculateNoise()
{
  /*B2INFO("Calculating noise");
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
  }*/
}

void DEPFETReaderModule::initialize()
{
  //Initialize PXDDigits collection
  StoreArray<PXDDigit> PXDDigits;

  if (m_inputFiles.empty()) {
    B2ERROR("No input files specified");
    return;
  }


  /*//calculate pedestals
  m_reader.open(m_inputFiles, m_calibrationEvents);
  m_reader.skip(m_skipEvents);
  calculatePedestals();

  //calculate noise
  m_reader.open(m_inputFiles, m_calibrationEvents);
  m_reader.skip(m_skipEvents);
  calculateNoise();*/

  //Read calibration files
  m_reader.setReadoutFold(m_readoutFold);
  if (m_dcd > 0) {
    m_commonMode = DEPFET::CommonMode(4, 1, 1, 1);
    m_reader.setTrailingFrames(m_trailingFrames);
  }
  m_reader.open(m_inputFiles);
  if (!m_reader.next()) {
    B2FATAL("Could not read a single event from the file");
  }
  DEPFET::Event& event = m_reader.getEvent();
  ADCValues& data = event[0];
  m_mask.setSize(data);
  m_pedestals.setSize(data);
  m_noise.setSize(data);

  //Read calibration data
  if (!m_calibrationFile.empty()) {
    ifstream maskStream(m_calibrationFile.c_str());
    if (!maskStream) {
      B2FATAL("Could not open calibration file " << m_calibrationFile);
    }
    while (maskStream) {
      int col, row, mask;
      double pedestal, noise;
      maskStream >> col >> row >> mask >> pedestal >> noise;
      if (!maskStream) break;
      m_mask(col, row) = mask;
      m_pedestals(col, row) = pedestal;
      m_noise(col, row) = noise;
    }
  }

  //Open file again
  m_reader.open(m_inputFiles);
  m_reader.skip(m_skipEvents);

  m_commonMode.setMask(&m_mask);
  m_commonMode.setNoise(m_sigmaCut, &m_noise);
  m_currentFrame = event.size();
}


void DEPFETReaderModule::event()
{
  StoreArray<PXDDigit>   storeDigits;
  const VXD::SensorInfoBase& info = VXD::GeoCache::get(VxdID(1, 1, 1));

  Event& event = m_reader.getEvent();

  //Get next event if we read all frames
  if (m_currentFrame >= event.size()) {
    if (!m_reader.next()) {
      StoreObjPtr <EventMetaData> eventMetaDataPtr;
      eventMetaDataPtr->setEndOfData();
      return;
    }
    m_currentFrame = 0;
  }

  ADCValues& data = event[m_currentFrame++];
  data.substract(m_pedestals);
  m_commonMode.apply(data);
  for (size_t y = 0; y < data.getSizeY(); ++y) {
    for (size_t x = 0; x < data.getSizeX(); ++x) {
      if (m_mask(x, y)) continue;
      double signal = data(x, y);
      if (signal < m_sigmaCut * m_noise(x, y)) continue;
      //Create new digit
      int digIndex = storeDigits->GetLast() + 1;
      new(storeDigits->AddrAt(digIndex)) PXDDigit(VxdID(1, 1, 1), x, y, info.getUCellPosition(x), info.getVCellPosition(y), max(0.0, signal));
    }
  }
}
