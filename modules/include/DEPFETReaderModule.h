/**************************************************************************
 * BASF2 (Belle Analysis Framework 2)                                     *
 * Copyright(C) 2010-2011  Belle II Collaboration                         *
 *                                                                        *
 * Author: The Belle II Collaboration                                     *
 * Contributors: Martin Ritter, Susanne Koblitz                           *
 *                                                                        *
 * This software is provided "as is" without any warranty.                *
 **************************************************************************/

#ifndef DEPFETREADERMODULE_H
#define DEPFETREADERMODULE_H

#include <framework/core/Module.h>

#include <string>
#include <generators/dataobjects/MCParticle.h>
#include <generators/dataobjects/MCParticleGraph.h>
#include <generators/hepevt/HepevtReader.h>

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
    void calibrate();
    void createPedestals

    std::vector <std::string>  m_inputFiles;
    int m_readoutFold;
    int m_calibrationEvents;
    int m_sigmaCut;
    int m_skipEvents;
    int m_borderSize;
  };

} // end namespace Belle2

#endif // DEPFETREADERMODULE_H
