/**************************************************************************
 * BASF2 (Belle Analysis Framework 2)                                     *
 * Copyright(C) 2010-2011  Belle II Collaboration                         *
 *                                                                        *
 * Author: The Belle II Collaboration                                     *
 * Contributors: Martin Ritter                                            *
 *                                                                        *
 * This software is provided "as is" without any warranty.                *
 **************************************************************************/

#ifndef DEPFETREADERMODULE_H
#define DEPFETREADERMODULE_H

#include <framework/core/Module.h>
#include <string>

#include <DEPFETReader/DataReader.h>
#include <DEPFETReader/CommonMode.h>
#include <DEPFETReader/IncrementalMean.h>
namespace DEPFET {
  typedef ValueMatrix<double> Pedestals;
  typedef ValueMatrix<double> Noise;
}

namespace Belle2 {

  /** The DEPFETReader module.
   * Loads events from a HepEvt file and stores the content
   * into the MCParticle class.
   */
  class DEPFETReaderModule : public Module {

  public:

    /**
     * Constructor.
     * Sets the module parameters.
     */
    DEPFETReaderModule();

    /** Destructor. */
    virtual ~DEPFETReaderModule() {}

    /** Initializes the module. */
    virtual void initialize();

    /** Method is called for each event. */
    virtual void event();

  protected:
    void progress(int event, int maxOrder=4);
    void calculatePedestals();
    void calculateNoise();

    std::vector<std::string> m_inputFiles;
    std::string m_calibrationFile;
    int m_readoutFold;
    int m_calibrationEvents;
    double m_sigmaCut;
    int m_skipEvents;
    int m_dcd;
    int m_trailingFrames;
    int m_currentFrame;

    DEPFET::DataReader m_reader;
    DEPFET::Pedestals m_pedestals;
    DEPFET::Noise m_noise;
    DEPFET::PixelMask m_mask;
    DEPFET::CommonMode m_commonMode;
  };

} // end namespace Belle2

#endif // DEPFETREADERMODULE_H
