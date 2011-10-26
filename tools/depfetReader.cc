#include <DEPFETReader/DataReader.h>

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

  class IncrementalMean {
    public:
      IncrementalMean(): m_entries(0), m_mean(0), m_variance(0) {}
      void clear(){
        m_entries = 0;
        m_mean = 0;
        m_variance = 0;
      }
      void add(double x, double weight=1.0, double sigmaCut=0.0){
        if(sigmaCut>0 && std::fabs(x-getMean())>getSigma()*sigmaCut) return;
        m_entries += weight;
        const double oldMean = m_mean;
        m_mean += weight*(x-oldMean)/m_entries;
        m_variance += weight*(x-oldMean)*(x-m_mean);
      }
      double getEntries() const { return m_entries; }
      double getMean() const { return m_mean; }
      double getSigma() const { return std::sqrt(m_variance/m_entries); }
    protected:
      double m_entries;
      double m_mean;
      double m_variance;
  };

  class DiscreteHistogram {
    public:
      DiscreteHistogram(): m_entries(0), m_mean(0), m_variance(0) {}

      void clear(){
        m_data.clear();
        m_entries = 0;
        m_mean = 0;
        m_variance = 0;
      }

      void add(int x){
        m_data[x]++;
        update(x);
      }

      int getEntries() const { return m_entries; }
      double getMean() const { return m_mean; }
      double getSigma() const { return std::sqrt(m_variance/m_entries); }

      void cutTails(double nSigma){
        double mean = getMean();
        double sigma = getSigma();

        m_entries = 0;
        m_mean = 0;
        m_variance = 0;
        for(std::map<int,int>::iterator it=m_data.begin(); it!=m_data.end();){
          if(std::fabs(it->first - mean)>sigma*nSigma){
            m_data.erase(it++);
          }else{
            update(it->first,it->second);
            ++it;
          }
        }
      }
    protected:
      void update(int x, int n=1){
        m_entries += n;
        double oldMean = m_mean;
        m_mean += n*(x-oldMean)/m_entries;
        m_variance += n*(x-oldMean)*(x-m_mean);
      }

      std::map<int,int> m_data;
      int m_entries;
      double m_mean;
      double m_variance;
  };

  class CommonMode {
    public:
      CommonMode():m_nX(0),m_nY(0){}
      void calculate(const ADCValues &data, const ValueMatrix<IncrementalMean>& pedestals, double nSigma);
      double operator()(int x, int y){
        int xhalf = 2*x/m_nX;
        return m_commonModeX[x].getMean() + m_commonModeY[(xhalf*m_nY + y)/2].getMean();
      }
    protected:
      int m_nX;
      int m_nY;
      std::vector<IncrementalMean> m_commonModeX;
      std::vector<IncrementalMean> m_commonModeY;
  };

  void CommonMode::calculate(const ADCValues &data, const ValueMatrix<IncrementalMean>& pedestals, double nSigma){
    m_nX = data.getSizeX();
    m_nY = data.getSizeY();
    m_commonModeX.clear();
    m_commonModeX.resize(m_nX);
    m_commonModeY.clear();
    m_commonModeY.resize(m_nY);

    for(int dir=0; dir<2; ++dir){
      for(int x=0; x<m_nX; ++x){
        int xhalf = 2*x/m_nX;
        for(int y=0; y<m_nY; ++y){
          double signal = data(x,y) - pedestals(x,y).getMean();
          if(std::fabs(signal)>nSigma*pedestals(x,y).getSigma()) continue;

          IncrementalMean &cmX =  m_commonModeX[x];
          IncrementalMean &cmY =  m_commonModeY[(xhalf*m_nY + y)/2];
          if(dir==1){
            cmX.add(signal-cmY.getMean());
          }else{
            cmY.add(signal-cmX.getMean());
          }
        }
      }
    }
  }

  ValueMatrix<IncrementalMean> calculatePedestals(DataReader &reader, int nInitial=20, double sigmaCut=3.0){
    ValueMatrix<IncrementalMean> pedestals;
    int eventNr(1);
    double cutValue(0.0);
    while(reader.next()){
      const DEPFET::Event& event = reader.getEvent();
      const DEPFET::ADCValues& data = event[0];
      if(eventNr==1) pedestals.setSize(data);
      if(eventNr==nInitial) cutValue = sigmaCut;
      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          pedestals(x,y).add(data(x,y),1.0,cutValue);
        }
      }
      progress(eventNr++,4);
    }
    return pedestals;
  }
}


/*std::pair<TH2D*,TH2D*> calculateMeanSigma(DEPFET::DataReader &reader, const std::string& name, TH2D* h_lastMean = 0, TH2D* h_lastSigma = 0, double nSigma=3.0)
{
  TH2D* h_mean(0);
  TH2D* h_sigma(0);
  TH2D* h_entries(0);

  int eventNr(1);
  while(reader.next()){
    const DEPFET::Event& event = reader.getEvent();
    const DEPFET::ADCValues& data = event[0];
    if(!h_mean){
      h_mean = new TH2D(name.c_str(),"Mean;row;column",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      h_sigma = new TH2D((name+"Sigma").c_str(),"Sigma;row;column",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      h_entries = new TH2D((name+"Entries").c_str(),"Entries;row;column",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
    }
    for(size_t x=0; x<data.getSizeX(); ++x){
      for(size_t y=0; y<data.getSizeY(); ++y){
        int adc = data(x,y);
        //check if we have a previous run and skip if more than 3 sigma away
        if(h_lastMean && h_lastSigma){
          double lastMean = h_lastMean->GetBinContent(x+1,y+1);
          double lastSigma = h_lastSigma->GetBinContent(x+1,y+1);
          if(std::fabs(adc-lastMean)>nSigma*lastSigma) continue;
        }
        double entries = h_entries->GetBinContent(x+1,y+1)+1;
        double oldMean = h_mean->GetBinContent(x+1,y+1);
        h_mean->Fill(x,y,(adc-oldMean)/entries);
        h_sigma->Fill(x,y,(adc-oldMean)*(adc-h_mean->GetBinContent(x+1,y+1)));
        h_entries->Fill(x,y);
      }
    }
    progress(eventNr++,4);
  }
  for(int x=1; x<=h_sigma->GetNbinsX(); ++x){
    for(int y=1; y<=h_sigma->GetNbinsY(); ++y){
      double entries = h_entries->GetBinContent(x,y);
      double variance = h_sigma->GetBinContent(x,y)/entries;
      h_sigma->SetBinContent(x,y,std::sqrt(variance));
    }
  }
  //delete h_entries;
  return std::make_pair(h_mean,h_sigma);
}*/

std::pair<TH2D*,TH2D*> calculateNoise(DEPFET::DataReader &reader, const std::string& name, const DEPFET::ValueMatrix<DEPFET::IncrementalMean> &pedestals, double nSigma=3.0, double cnmSigma=4.0)
{
  TH2D* h_mean(0);
  TH2D* h_sigma(0);
  TH2D* h_entries(0);

  DEPFET::CommonMode cMode;

  int eventNr(1);
  while(reader.next()){
    const DEPFET::Event& event = reader.getEvent();
    const DEPFET::ADCValues& data = event[0];
    cMode.calculate(data,pedestals,cnmSigma);
    if(!h_mean){
      h_mean = new TH2D(name.c_str(),"Mean;column;row",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      h_sigma = new TH2D((name+"Sigma").c_str(),"Sigma;column;row",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      h_entries = new TH2D((name+"Entries").c_str(),"Entries;column;row",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
    }
    for(size_t x=0; x<data.getSizeX(); ++x){
      for(size_t y=0; y<data.getSizeY(); ++y){
        double pedestal = pedestals(x,y).getMean();
        double sigma = pedestals(x,y).getSigma();
        double signal = data(x,y) - pedestal - cMode(x,y);
        if(std::fabs(signal)>nSigma*sigma) continue;

        double entries = h_entries->GetBinContent(x+1,y+1)+1;
        double oldMean = h_mean->GetBinContent(x+1,y+1);
        h_mean->Fill(x,y,(signal-oldMean)/entries);
        h_sigma->Fill(x,y,(signal-oldMean)*(signal-h_mean->GetBinContent(x+1,y+1)));
        h_entries->Fill(x,y);
      }
    }
    progress(eventNr++,4);
    //break;
  }
  for(int x=1; x<=h_sigma->GetNbinsX(); ++x){
    for(int y=1; y<=h_sigma->GetNbinsY(); ++y){
      double entries = h_entries->GetBinContent(x,y);
      double variance = h_sigma->GetBinContent(x,y)/entries;
      h_sigma->SetBinContent(x,y,std::sqrt(variance));
    }
  }
  //delete h_entries;
  return std::make_pair(h_mean,h_sigma);
}

void createSignalHist(DEPFET::DataReader &reader, const std::string& name, const DEPFET::ValueMatrix<DEPFET::IncrementalMean> &pedestals, TH2D* h_noise, double nSigma=3.0){
  TH2D* h_signal(0);
  DEPFET::CommonMode cMode;

  int eventNr(1);
  while(reader.next()){
    const DEPFET::Event& event = reader.getEvent();
    const DEPFET::ADCValues& data = event[0];
    cMode.calculate(data,pedestals,5.0);
    std::string name = (boost::format("signal%02d") % eventNr).str();
    h_signal = new TH2D(name.c_str(),"Signal;column;row",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
    for(size_t x=0; x<data.getSizeX(); ++x){
      for(size_t y=0; y<data.getSizeY(); ++y){
      double noise = h_noise->GetBinContent(x+1,y+1);
      double signal = data(x,y) - pedestals(x,y).getMean() - cMode(x,y);
      if(signal<nSigma*noise) continue;
      h_signal->Fill(x,y,signal/noise);
    }
    }
    progress(eventNr++,4);
    if(eventNr>=10) break;
  }
}

int main(int argc, char* argv[])
{
  DEPFET::ValueMatrix<DEPFET::IncrementalMean> npedestals;

  DEPFET::DataReader reader;
  reader.setReadoutFold(2);
  int nEvents = std::atoi(argv[1]);

  TFile* r_file = new TFile(argv[3],"RECREATE");
  TH2D *rawMean(0), *rawSigma(0);
  TH2D *pedestals(0), *pedestalsigma(0);
  TH2D *noiseMean(0), *noise(0);
  TH2D *pdiff(0);
  for(int pass=0; pass<5; pass++){
    reader.open(argv[2], 0, nEvents);
    reader.skip(1);
    switch(pass){
      /*case 0:
        std::cout << "Calculating mean pixel value" << std::endl;
        boost::tie(rawMean,rawSigma) = calculateMeanSigma(reader,"raw");
        break;
      case 1:
        std::cout << "Calculating pedestals" << std::endl;
        boost::tie(pedestals,pedestalsigma) = calculateMeanSigma(reader,"pedestal",rawMean,rawSigma,5.0);
        break;*/
      case 2:
        npedestals = calculatePedestals(reader,20,5.0);
        /*pdiff = new TH2D("pdiff","",npedestals.getSizeX(),0,npedestals.getSizeX(),npedestals.getSizeY(),0,npedestals.getSizeY());
        for(int x=0; x<npedestals.getSizeX(); ++x){
          for(int y=0; y<npedestals.getSizeY(); ++y){
            pdiff->SetBinContent(x+1,y+1,pedestals->GetBinContent(x+1,y+1) - npedestals(x,y).getMean());
          }
        }*/
        break;
      case 3:
        std::cout << "Calculating noise" << std::endl;
        boost::tie(noiseMean,noise) = calculateNoise(reader,"noise",npedestals, 5.0, 7.0);
        break;
      case 4:
        std::cout << "Plotting signal for 10 events" << std::endl;
        createSignalHist(reader,"noise",npedestals, noise, 5.0);
        break;
    }
  }
  r_file->Write();
  r_file->Close();
}

/*  TFile* r_lastRun(0);
  TH2D* h_lastMean(0);
  TH2D* h_lastSigma(0);
  if(argc>4){
    r_lastRun = new TFile(argv[4]);
    h_lastMean = dynamic_cast<TH2D*>(r_lastRun->Get("mean"));
    h_lastSigma = dynamic_cast<TH2D*>(r_lastRun->Get("sigma"));
  }

  TFile* r_file = new TFile(argv[3],"RECREATE");

  TH2D* h_mean(0);
  TH2D* h_variance(0);
  TH2D* h_sigma(0);
  TH2D* h_entries(0);
  TH2D* h_signal(0);
  int min = std::numeric_limits<int>::max();
  int max = std::numeric_limits<int>::min();
  int eventNr(1);
  while(reader.next()){
    const DEPFET::Event& event = reader.getEvent();
    const DEPFET::ADCValues& data = event[0];
    if(!h_mean){
      h_mean = new TH2D("mean","Mean;row;column",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      h_variance = new TH2D("variance","Variance;row;column",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      h_sigma = new TH2D("sigma","Sigma;row;column",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      h_entries = new TH2D("entries","Entries;row;column",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
      h_signal = new TH2D("signal","Entries;row;column",data.getSizeX(),0,data.getSizeX(),data.getSizeY(),0,data.getSizeY());
    }
    double commonModeValues[2][2] = {{0,0},{0,0}};
    int    commonModeEntries[2][2] = {{0,0},{0,0}};
    for(size_t x=0; x<data.getSizeX(); ++x){
      for(size_t y=0; y<data.getSizeY(); ++y){
        int adc = data(x,y);
        //check if we have a previous run and skip if more than 3 sigma away
        if(h_lastMean && h_lastSigma){
          double lastMean = h_lastMean->GetBinContent(x+1,y+1);
          double lastSigma = h_lastSigma->GetBinContent(x+1,y+1);
          if(std::fabs(adc-lastMean)>4*lastSigma) continue;
          //Common mode calculation for this frame taking all values between 4 sigma
          int halfX = 2*x/data.getSizeX();
          int halfY = 2*Y/data.getSizeX();
          commonModeValues[0][halfX] += (adc-lastMean-commonModeValues[0][halfX])/(++commonModeEntries[0][halfX]);
          commonModeValues[1][halfY] += (adc-lastMean-commonModeValues[1][halfY])/(++commonModeEntries[1][halfY]);
          if(std::fabs(adc-lastMean)>3*lastSigma) continue;
        }
        min = std::min(adc,min);
        max = std::max(adc,max);
        double entries = h_entries->GetBinContent(x+1,y+1)+1;
        double oldMean = h_mean->GetBinContent(x+1,y+1);
        h_mean->Fill(x,y,(adc-oldMean)/entries);
        h_variance->Fill(x,y,(adc-oldMean)*(adc-h_mean->GetBinContent(x+1,y+1)));
        h_entries->Fill(x,y);
      }
    }
    if(h_lastMean && h_lastSigma){
      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          int halfX = 2*x/data.getSizeX();
          int halfY
            int adc = data(x,y);



          progress(eventNr++,4);
        }
        for(int x=1; x<=h_variance->GetNbinsX(); ++x){
          for(int y=1; y<=h_variance->GetNbinsY(); ++y){
            double entries = h_entries->GetBinContent(x,y);
            double variance = h_variance->GetBinContent(x,y)/entries;
            h_variance->SetBinContent(x,y,variance);
            h_sigma->SetBinContent(x,y,std::sqrt(variance));
          }
        }
        h_mean->Write();
        h_variance->Write();
        h_sigma->Write();
        h_entries->Write();
        h_signal->Write();
        r_file->Write();
        r_file->Close();
        std::cout << "Min: " << min << ", Max: " << max << std::endl;
        return 0;
      }*/

