#include <DEPFETReader/DataReader.h>
#include <DEPFETReader/CommonMode2.h>
#include <DEPFETReader/IncrementalMean.h>

#include <TFile.h>
#include <TH2D.h>
#include <TH2I.h>

#include <cmath>
#include <limits>
#include <cstdlib>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>

void progress(int event, int maxOrder=4){
  int order = (event == 0) ? 1 : static_cast<int>(std::min(std::log10(event), (double)maxOrder));
  int interval = static_cast<int>(std::pow(10., order));
  if (event % interval == 0) std::cout << "Events read: " << event << std::endl;
}

namespace DEPFET {
  typedef ValueMatrix<IncrementalMean> Pedestals;
  typedef ValueMatrix<double> Noise;

  Pedestals calculatePedestals(DataReader &reader, const Pedestals &firstRun, double sigmaCut=3.0){
    ValueMatrix<IncrementalMean> pedestals;
    int eventNr(1);
    while(reader.next()){
      const Event& event = reader.getEvent();
      const ADCValues& data = event[0];
      if(eventNr==1) pedestals.setSize(data);
      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          if(sigmaCut>0){
            double mean = firstRun(x,y).getMean();
            double width = firstRun(x,y).getSigma()*sigmaCut;
            if(std::fabs(data(x,y)-mean)>width) continue;
          }
          pedestals(x,y).add(data(x,y));
        }
      }
      progress(eventNr++,4);
    }
    return pedestals;
  }

  template<class T> TH2D* createHist(const std::string& name, const std::string& title, const ValueMatrix<T>& data){
    TH2D* hist = new TH2D(name.c_str(),title.c_str(),
        data.getSizeX(),0,data.getSizeX(),
        data.getSizeY(),0,data.getSizeY());
    for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          hist->Fill(x,y,(double) data(x,y));
        }
    }
    return hist;
  }

  Noise calculateNoise(DataReader &reader, const Pedestals &pedestals, double sigmaCut=3.0){
    ValueMatrix<IncrementalMean> noise;
    noise.setSize(pedestals);
    CommonMode cMode(2,1,2,1);
    int eventNr(1);
    while(reader.next()){
      Event& event = reader.getEvent();
      ADCValues& data = event[0];
      data.substract(pedestals);
      cMode.apply(data);
      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          double signal = data(x,y);
          if(std::fabs(signal)>sigmaCut*pedestals(x,y).getSigma()) continue;
          noise(x,y).add(signal);
        }
      }
      progress(eventNr++,4);
    }

    Noise result;
    result.setSize(pedestals);
    createHist("noiseMean","Noise Mean",noise);
    for(size_t x=0; x<noise.getSizeX(); ++x){
      for(size_t y=0; y<noise.getSizeY(); ++y){
        result(x,y) = noise(x,y).getSigma();
      }
    }
    return result;
  }

  void createSignalHist(DataReader &reader, const Pedestals &pedestals, const Noise &noise, double nSigma=3.0){
    TH2D* h_signal(0);
    TH2D* h_hitmap(0);
    TH1D* h_commonModeRow = new TH1D("cmRow","Row wise Common Mode; adu;",200,-100,100);
    TH1D* h_commonModeCol = new TH1D("cmCol","Col wise Common Mode; adu;",40,-20,20);
    DEPFET::CommonMode cMode(2,1,2,1);

    int eventNr(1);
    while(reader.next()){
      Event& event = reader.getEvent();
      ADCValues& data = event[0];
      data.substract(pedestals);
      cMode.apply(data);
      const std::vector<int>& cmRow = cMode.getCommonModesRow();
      const std::vector<int>& cmCol = cMode.getCommonModesCol();
      for(size_t i=0; i<cmRow.size(); ++i) h_commonModeRow->Fill(cmRow[i]);
      for(size_t i=0; i<cmCol.size(); ++i) h_commonModeCol->Fill(cmCol[i]);
      if(!h_hitmap) h_hitmap = new TH2D("hitmap","Hitmap;column;row",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      if((eventNr-1)%100==0) {
        std::string name = (boost::format("signal%02d") % eventNr).str();
        h_signal = new TH2D(name.c_str(),"Signal;column;row",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      }
      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          double signal = data(x,y);
          if(signal<nSigma*noise(x,y)) continue;
          h_signal->Fill(x,y,signal/noise(x,y));
          h_hitmap->Fill(x,y);
        }
      }
      progress(eventNr++,4);
      //if(eventNr>=10) break;
    }
  }

}


int main(int argc, char* argv[])
{
  DEPFET::Pedestals pedestals;
  DEPFET::ValueMatrix<double> noise;
  DEPFET::DataReader reader;
  reader.setReadoutFold(2);
  TH2D* h_pedestals(0);
  //TH2D* h_pedestalSpread(0);
  TH2D* h_noise(0);

  TFile* rootFile = new TFile(argv[3],"RECREATE");

  int nEvents = std::atoi(argv[1]);
  for(int pass=0; pass<4; pass++){
    if(pass>2) nEvents=0;
    reader.open(argv[2], 0, nEvents);
    reader.skip(1);
    switch(pass){
      case 0:
        std::cout << "Calculating pixel mean" << std::endl;
        pedestals = calculatePedestals(reader, pedestals, 0);
        DEPFET::createHist("raw","Raw pixel data",pedestals);
        break;
      case 1:
        std::cout << "Calculating pedestals" << std::endl;
        pedestals = calculatePedestals(reader, pedestals, 3.0);
        h_pedestals = DEPFET::createHist("pedestals","Pedestals",pedestals);
        break;
      case 2:
        std::cout << "Calculating noise" << std::endl;
        noise = calculateNoise(reader,pedestals, 5.0);
        h_noise = DEPFET::createHist("noise","Noise",noise);
        break;
      case 3:
        std::cout << "Make Signal hist" << std::endl;
        createSignalHist(reader,pedestals,noise,3.0);
        break;
    }
  }
  rootFile->Write();
  rootFile->Close();
}
