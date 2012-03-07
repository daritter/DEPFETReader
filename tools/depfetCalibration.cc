#include <DEPFETReader/DataReader.h>
#include <DEPFETReader/CommonMode.h>
#include <DEPFETReader/IncrementalMean.h>

#include <cmath>
#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/char_traits.hpp> // EOF, WOULD_BLOCK
#include <boost/iostreams/concepts.hpp>    // input_filter
#include <boost/iostreams/operations.hpp>  // get
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <TH1D.h>
#include <TFile.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TF1.h>
#include <TGraph.h>
#include <TMath.h>

using namespace std;
namespace po = boost::program_options;
namespace io = boost::iostreams;

class CommentFilter : public boost::iostreams::input_filter {
  public:
    explicit CommentFilter(char commentChar = '#'): m_commentChar(commentChar), m_skip(false)
  {}

    template<typename Source>
      int get(Source& src) {
        int c;
        while (true) {
          c = boost::iostreams::get(src);
          if (c == EOF || c == boost::iostreams::WOULD_BLOCK){
            break;
          }
          if(c == m_commentChar){
            m_skip = true;
          } else if(c == '\n'){
            m_skip = false;
          }

          if (!m_skip) break;
        }
        return c;
      }

    template<typename Source>
      void close(Source&) { m_skip = false; }
  private:
    char m_commentChar;
    bool m_skip;
};

double setRangeFraction(TH1* hist, double fraction=0.9){
  double mean = hist->GetMean();
  double maxEntries = hist->GetEntries()*fraction;
  if(maxEntries < 1 ) {
    cerr << hist->GetName() << ": Not enough Entries (" << hist->GetEntries() << ") for RangeFraction ("<< fraction << ")\n";
    return 0;
  }
  int binLeft = hist->FindBin(mean);
  int binRight = binLeft;
  int binMax = hist->GetNbinsX();
  double entries = hist->GetBinContent(binLeft);
  while (entries < maxEntries){
    if(--binLeft>0){
      entries += hist->GetBinContent(binLeft);
    }
    if(++binRight<=binMax){
      entries += hist->GetBinContent(binRight);
    }
    if (binLeft <= 0 && binRight > binMax) {
      cerr << "Did not reach " << fraction << " with " << hist->GetName() << std::endl;
      break;
    }
  }
  hist->GetXaxis()->SetRange(binLeft,binRight);
  return entries/hist->GetEntries();
}


bool showProgress(int event, int minOrder=1, int maxOrder=3){
  int order = (event == 0) ? 1 : max(min( (int)log10(event), maxOrder), minOrder);
  int interval = static_cast<int>(pow(10., order));
  return (event % interval == 0);
}

typedef DEPFET::ValueMatrix<DEPFET::IncrementalMean> PixelMean;
typedef DEPFET::ValueMatrix<TH1D*> HistGrid;
typedef DEPFET::ValueMatrix<TGraph*> GraphGrid;

//Output a single value to file
inline void dumpValue(ostream& output, double value, double scale){
  if(isnan(value)) value=0;
  if(output) output << setprecision(2) << setw(8) << fixed << (value*scale) << " ";
}

//Calculate the pedestals: Determine mean and sigma of every pixel, optionally
//applying a cut using mean and sigma of a previous run
void calculatePedestals(DEPFET::DataReader &reader, PixelMean &pedestals, double sigmaCut, DEPFET::PixelMask &masked){
  PixelMean newPedestals;
  int eventNr(1);
  while(reader.next()){
    DEPFET::Event &event = reader.getEvent();
    BOOST_FOREACH(DEPFET::ADCValues &data, event){
      if(!newPedestals) newPedestals.setSize(data);

      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          if(masked(x,y)) continue;
          if(sigmaCut>0){
            const double mean = pedestals(x,y).getMean();
            const double width = pedestals(x,y).getSigma()*sigmaCut;
            if(std::fabs(data(x,y)-mean)>width) continue;
          }
          newPedestals(x,y).add(data(x,y));
        }
      }
    }
    if(showProgress(eventNr)){
      cout << "Pedestal calculation (" << sigmaCut << " sigma cut): " << eventNr << " events read" << endl;
    }
    ++eventNr;
  }
  swap(newPedestals,pedestals);
}

int main(int argc, char* argv[])
{
  int skipEvents(0);
  int maxEvents(-1);
  double sigmaCut(5.0);
  double scaleFactor(1.0);
  vector<string> inputFiles;
  string outputFile;
  string maskFile("MaskCh-Mod%1%.txt");

  //Parse program arguments
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "Show help message")
    ("sigma", po::value<double>(&sigmaCut)->default_value(5.0), "sigma cut")
    ("skip,s", po::value<int>(&skipEvents)->default_value(0),"Number of events to skip before reading")
    ("nevents,n", po::value<int>(&maxEvents)->default_value(-1),"Max. number of output events")
    ("mask", po::value<string>(&maskFile), "Filename for reading pixel masks")
    ("input,i", po::value< vector<string> >(&inputFiles)->composing(),"Input files")
    ("output,o", po::value<string>(&outputFile)->default_value("output.dat"), "Output file")
    ("scale", po::value<double>(&scaleFactor)->default_value(1.0), "Scaling factor for ADC values")
    ("4fold", "If set, data is read out in 4fold mode, otherwise 2fold")
    ("dcd", "If set, common mode corretion is set to DCD mode (4 full rows), otherwise curo topology is used (two half rows")
    ("trailing", po::value<int>(), "Set number of trailing frames")
    ;

  po::variables_map vm;
  po::positional_options_description p;
  p.add("input", -1);

  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
  po::notify(vm);
  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  //check program arguments
  if(inputFiles.empty()) {
    cerr << "No input files given" << endl;
    return 2;
  }

  DEPFET::DataReader reader;
  reader.setReadoutFold(2);
  reader.setUseDCDBMapping(true);
  //Common mode correction: row wise correction using two half rows and one column
  DEPFET::CommonMode commonMode(2,1,2,1);
  if(vm.count("4fold")) {
    reader.setReadoutFold(4);
  }
  if(vm.count("dcd")){
    commonMode = DEPFET::CommonMode(4,1,1,1);
  }
  if(vm.count("trailing")){
    reader.setTrailingFrames(vm["trailing"].as<int>());
  }

  PixelMean pedestals;
  HistGrid noise;
  GraphGrid raw;
  DEPFET::PixelMask  masked;

  reader.open(inputFiles,maxEvents);
  if(!reader.next()){
    cerr << "Could not read a single event from the file" << cerr;
    return 5;
  }
  DEPFET::Event &event = reader.getEvent();
  boost::format maskFileFormat(maskFile);
  DEPFET::ADCValues &data = event[0];
  noise.setSize(data);
  raw.setSize(data);
  masked.setSize(data);
  if(!maskFile.empty()){
    io::filtering_istream maskStream;
    maskStream.push(CommentFilter());
    maskStream.push(io::file_source((maskFileFormat%data.getModuleNr()).str()));
    //ifstream maskStream((maskFileFormat%data.getModuleNr()).str().c_str());
    if(!maskStream){
      cerr << "Could not open mask file for module " << data.getModuleNr() << ", not masking any pixels" << endl;
    }
    //Skip first line
    if(maskStream.peek() == '*') {
      maskStream.ignore(1000,'\n');
    }
    //Use the rest for masking
    while(maskStream){
      int col, row;
      maskStream >> col >> row;
      if(!maskStream) break;
      if(col<0) for(col=0;col<(int)masked.getSizeX();++col) {
        masked(col,row)=1;
      }else  if(row<0) for(row=0;row<(int)masked.getSizeY();++row) {
        masked(col,row)=1;
      }else
        masked(col,row)=1;
    }
  }

  gStyle->SetOptFit(11111);
  boost::format name("noise-%02dx%02d");
  for(unsigned int col=0; col<noise.getSizeX(); ++col){
    for(unsigned int row=0; row<noise.getSizeY(); ++row){
      noise(col,row) = new TH1D((name % col % row).str().c_str(),"",80,-10,10);
      raw(col,row) = new TGraph(1000);
    }
  }

  //Calibration: Calculate pedestals, first run: determine mean and sigma for each pixel
  reader.open(inputFiles,maxEvents);
  reader.skip(skipEvents);
  calculatePedestals(reader, pedestals, 0, masked);

  //Second run, this time exclude values outside of sigmaCut * pedestal spread
  reader.open(inputFiles,maxEvents);
  reader.skip(skipEvents);
  calculatePedestals(reader, pedestals, sigmaCut, masked);

  //Third run to determine noise level of pixels
  reader.open(inputFiles,maxEvents);
  reader.skip(skipEvents);
  int eventNr(1);
  TH1D* noiseFitProb = new TH1D("noiseFitPval","NoiseFit p-value;p-value,#",100,0,1);
  TH1D* cMRHist = new TH1D("commonModeR","common mode, row wise",160,-20,20);
  TH1D* cMCHist = new TH1D("commonModeC","common mode, column wise",160,-20,20);
  commonMode.setMask(&masked);
  while(reader.next()){
    DEPFET::Event &event = reader.getEvent();
    BOOST_FOREACH(DEPFET::ADCValues &data, event){
      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          raw(x,y)->SetPoint(eventNr-1,eventNr,data(x,y));
        }
      }
      //Pedestal substraction
      data.substract(pedestals);
      //Common Mode correction
      commonMode.apply(data);
      BOOST_FOREACH(double c, commonMode.getCommonModesRow()){
        cMRHist->Fill(c);
      }
      BOOST_FOREACH(double c, commonMode.getCommonModesCol()){
        cMCHist->Fill(c);
      }

      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          if(masked(x,y)) continue;
          double signal = data(x,y);
          //Add signal to noise map if it is below nSigma*(sigma of pedestal)
          if(std::fabs(signal)>sigmaCut*pedestals(x,y).getSigma()) continue;
          noise(x,y)->Fill(signal);
        }
      }
      if(showProgress(eventNr)){
        cout << "Calculating Noise: " << eventNr << " events read" << endl;
      }
      ++eventNr;
    }
  }


  ofstream output(outputFile.c_str());
  if(!output){
    cerr << "Could not open output file " << outputFile << endl;
    return 3;
  }
  TFile* rootFile = new TFile("noise.root","RECREATE");
  TCanvas* c1 = new TCanvas("c1","c1");
  TF1* func = new TF1("f1","gaus");
  c1->cd();
  boost::format canvasName("noiseFit-%02dx%02d");
  for(unsigned int col=0; col<pedestals.getSizeX(); ++col){
    for(unsigned int row=0; row<pedestals.getSizeY(); ++row){
      output << setw(6) << col << setw(6) << row << setw(2) << (int)masked(col,row) << " ";
      dumpValue(output, pedestals(col,row).getMean(), scaleFactor);
      noise(col,row)->Draw();
      if(masked(col,row)){
        dumpValue(output,0,0);
      }else{
        setRangeFraction(noise(col,row));
        noise(col,row)->Fit(func);
        noiseFitProb->Fill(TMath::Prob(func->GetChisquare(),func->GetNDF()));
        dumpValue(output, func->GetParameter(2), scaleFactor);
        //func->Draw("same");
        c1->Update();
        c1->Write((canvasName % col % row).str().c_str());
      }
      output << endl;
    }
  }
  output.close();
  noiseFitProb->Write();
  cMRHist->Write();
  cMCHist->Write();

  boost::format rawName("raw-%02dx%02d");
  for(size_t x=0; x<raw.getSizeX(); ++x){
    for(size_t y=0; y<raw.getSizeY(); ++y){
      raw(x,y)->Write((rawName % x % y).str().c_str());
    }
  }

  rootFile->Write();
  rootFile->Close();
}
