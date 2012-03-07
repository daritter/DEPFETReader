#include <DEPFETReader/DataReader.h>
#include <DEPFETReader/CommonMode.h>
#include <DEPFETReader/IncrementalMean.h>

#include <cmath>
#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <algorithm>

using namespace std;
namespace po = boost::program_options;

bool showProgress(int event, int minOrder = 0, int maxOrder = 3)
{
  int order = (event == 0) ? 1 : max(min((int)log10(event), maxOrder), minOrder);
  int interval = static_cast<int>(pow(10., order));
  return (event % interval == 0);
}

struct pixel {
  pixel(int x, int y, double adc): x(x), y(y), adc(adc) {}
  unsigned short x;
  unsigned short y;
  double adc;
  bool operator<(const pixel& b) const { return adc < b.adc; }
};

typedef DEPFET::ValueMatrix<double> PixelValues;

int main(int argc, char* argv[])
{
  int skipEvents(0);
  int maxEvents(-1);
  vector<string> inputFiles;
  string outputFile;
  string calibrationFile;
  double sigmaCut(5.0);

  int minU, maxU, minV, maxV;

  //Parse program arguments
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", "Show help message")
  ("skip,s", po::value<int>(&skipEvents)->default_value(0), "Number of events to skip before reading")
  ("sigma", po::value<double>(&sigmaCut)->default_value(5.0), "Sigma cut to apply to seed")
  ("nevents,n", po::value<int>(&maxEvents)->default_value(-1), "Max. number of output events")
  ("calibration,c", po::value<string>(&calibrationFile), "Calibration File")
  ("input,i", po::value< vector<string> >(&inputFiles)->composing(), "Input files")
  ("output,o", po::value<string>(&outputFile)->default_value("clusters.root"), "Output file")
  ("4fold", "If set, data is read out in 4fold mode, otherwise 2fold")
  ("dcd", "If set, common mode corretion is set to DCD mode (4 full rows), otherwise curo topology is used (two half rows")
  ("trailing", po::value<int>(), "Set number of trailing frames")
  ("minU", po::value<int>(&minU)->default_value(0), "minimal U coordinate to consider")
  ("maxU", po::value<int>(&maxU)->default_value(0), "maximal U coordinate to consider")
  ("minV", po::value<int>(&minV)->default_value(-1), "minimal V coordinate to consider")
  ("maxV", po::value<int>(&maxV)->default_value(-1), "maximal V coordinate to consider")
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
  if (inputFiles.empty()) {
    cerr << "No input files given" << endl;
    return 2;
  }

  DEPFET::DataReader reader;
  reader.setReadoutFold(2);
  reader.setUseDCDBMapping(true);
  //Common mode correction: row wise correction using two half rows and one column
  DEPFET::CommonMode commonMode(2, 1, 2, 1);
  if (vm.count("4fold")) {
    reader.setReadoutFold(4);
  }
  if (vm.count("dcd")) {
    commonMode = DEPFET::CommonMode(4, 1, 1, 1);
  }
  if (vm.count("trailing")) {
    reader.setTrailingFrames(vm["trailing"].as<int>());
  }

  reader.open(inputFiles, maxEvents);
  if (!reader.next()) {
    cerr << "Could not read a single event from the file" << cerr;
    return 5;
  }

  if (calibrationFile.empty()) {
    cerr << "No calibration file given" << endl;
    return 4;
  }
  ifstream calStream(calibrationFile.c_str());
  if (!calStream) {
    cerr << "Could not open calibration file " << calibrationFile << endl;
    return 5;
  }

  DEPFET::PixelMask mask;
  PixelValues pedestals;
  PixelValues noise;
  PixelValues hitmap;

  DEPFET::Event& event = reader.getEvent();
  mask.setSize(event[0]);
  pedestals.setSize(mask);
  hitmap.setSize(mask);
  noise.setSize(mask);
  if (maxU < 0) maxU = mask.getSizeX();
  if (maxV < 0) maxV = mask.getSizeY();

  while (calStream) {
    int col, row, px_masked;
    double px_pedestal, px_noise;
    calStream >> col >> row >> px_masked >> px_pedestal >> px_noise;
    if (!calStream) break;
    mask.at(col, row) = px_masked;
    pedestals.at(col, row) = px_pedestal;
    noise.at(col, row) = px_noise;
  }

  TH1::SetDefaultBufferSize(10000);
  TFile* rootFile = new TFile(outputFile.c_str(), "RECREATE");
  TH1D* clsEnergy = new TH1D("clusterCharge", "Cluster Charge;ADU", 100, 0, 0);
  TH2D* seedPos   = new TH2D("seedPos", "Seed position;col;row", maxU - minU, minU, maxU, maxV - minV, minV, maxV);
  TH1D* seedEnergy = new TH1D("seedCharge", "Seed Charge;ADU", 100, 0, 0);
  TH1D* pixelSum  = new TH1D("pixelSum", "Pixel Sum;ADU", 200, 0, 0);
  TH1D* nClusters = new TH1D("nClusters", "Number of Clusters", 100, 0, 0);

  commonMode.setMask(&mask);
  commonMode.setNoise(sigmaCut, &noise);
  int eventNr(1);
  reader.open(inputFiles, maxEvents);
  reader.skip(skipEvents);
  std::vector<pixel> seeds;
  while (reader.next()) {
    DEPFET::Event& event = reader.getEvent();
    BOOST_FOREACH(DEPFET::ADCValues & data, event) {
      // DEPFET::ADCValues &data = event[0];
      //Pedestal substraction
      data.substract(pedestals);
      //Common Mode correction
      commonMode.apply(data);
      seeds.clear();
      //At this point, data(x,y) is the pixel value of column x, row y
      double sum = 0;
      for (size_t y = minV; y < maxV; ++y) {
        for (size_t x = minU; x < maxU; ++x) {
          sum += data(x, y);
          if (data(x, y) > sigmaCut * noise(x, y)) {
            seeds.push_back(pixel(x, y, data(x, y)));
          }
        }
      }
      pixelSum->Fill(sum);
      //if(sum<0) continue;
      std::sort(seeds.begin(), seeds.end());
      int ncls(0);
      BOOST_FOREACH(pixel & px, seeds) {
        //highest two:
        if (data(px.x, px.y) <= 0) continue;
        data(px.x, px.y) = 0;
        pixel n(0, 0, 0);
        for (int i = -1; i < 2; ++i) {
          for (int j = -1; j < 2; ++j) {
            double signal = data(px.x + i, px.y + j);
            if (signal > 0) { //noise(px.x+i,px.y+j)){
              if (signal > n.adc) n = pixel(px.x + i, px.y + j, signal);
            }
          }
        }
        seedEnergy->Fill(px.adc);
        seedPos->Fill(px.x, px.y);
        if (n.adc > 0) {
          data(n.x, n.y) = 0;
          px.adc += n.adc;
        }
        clsEnergy->Fill(px.adc);
        ncls++;

        /*//Take seed and sum 3x3 around it
        if(data(px.x,px.y)<=0) continue;
        double charge = 0;
        for(int i=-1; i<2; ++i){
          for(int j=-1; j<2; ++j){
            if(data(px.x+i,px.y+j)>noise(px.x+i,px.y+j)) charge += data(px.x+i,px.y+j);
            data(px.x+i,px.y+i) = 0;
          }
        }
        //Check if still above threshold
        if(charge>sigmaCut*noise(px.x,px.y)){
          clsEnergy->Fill(charge);
          seedEnergy->Fill(px.adc);
          seedPos->Fill(px.x,px.y);
        }*/
      }
      nClusters->Fill(ncls);
    }
    if (showProgress(eventNr)) {
      cout << "Output: " << eventNr << " events written" << endl;
    }
    ++eventNr;
  }

  //clsEnergy->Write();
  //seedEnergy->Write();
  //
  clsEnergy->BufferEmpty();
  seedPos->BufferEmpty();
  seedEnergy->BufferEmpty();
  pixelSum->BufferEmpty();
  nClusters->BufferEmpty();

  rootFile->Write();
  rootFile->Close();
}
