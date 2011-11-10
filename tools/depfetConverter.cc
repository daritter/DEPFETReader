#include <DEPFETReader/DataReader.h>
#include <DEPFETReader/CommonMode.h>
#include <DEPFETReader/IncrementalMean.h>

#include <cmath>
#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

using namespace std;
namespace po = boost::program_options;

bool showProgress(int event, int minOrder=1, int maxOrder=3){
  int order = (event == 0) ? 1 : max(min( (int)log10(event), maxOrder), minOrder);
  int interval = static_cast<int>(pow(10., order));
  return (event % interval == 0);
}

typedef DEPFET::ValueMatrix<DEPFET::IncrementalMean> PixelMean;
typedef map<int, PixelMean> PixelMeanMap;

//Output a single value to file
inline void dumpValue(ostream& output, double value, double scale){
  if(output) output << setprecision(2) << setw(8) << fixed << (value*scale) << " ";
}

//Output a matrix of mean/sigma values for each module
void dumpValues(ostream& output, const PixelMeanMap &dataMap, double scaleFactor){
  output << "mean " << dataMap.size() << endl;
  BOOST_FOREACH(const PixelMeanMap::value_type &it, dataMap){
    const PixelMean &data = it.second;
    output << "module " << it.first << " " << data.getSizeX() << " " << data.getSizeY() << endl;
    for(size_t y=0; y<data.getSizeY(); ++y){
      for(size_t x=0; x<data.getSizeX(); ++x){
        dumpValue(output,data(x,y).getMean(),scaleFactor);
      }
      output << endl;
    }
  }
  output << endl;

  output << "sigma " << dataMap.size() << endl;
  BOOST_FOREACH(const PixelMeanMap::value_type &it, dataMap){
    const PixelMean &data = it.second;
    output << "module " << it.first << " " << data.getSizeX() << " " << data.getSizeY() << endl;
    for(size_t y=0; y<data.getSizeY(); ++y){
      for(size_t x=0; x<data.getSizeX(); ++x){
        dumpValue(output,data(x,y).getSigma(),scaleFactor);
      }
      output << endl;
    }
  }
  output << endl;
}

//Calculate the pedestals: Determine mean and sigma of every pixel, optionally
//applying a cut using mean and sigma of a previous run
void calculatePedestals(DEPFET::DataReader &reader, PixelMeanMap &pedestalMap, double sigmaCut){
  PixelMeanMap newPedestalMap;
  int eventNr(1);
  while(reader.next()){
    DEPFET::Event &event = reader.getEvent();
    BOOST_FOREACH(DEPFET::ADCValues &data, event){
      const PixelMean &previous = pedestalMap[data.getModuleNr()];
      PixelMean &pedestals = newPedestalMap[data.getModuleNr()];
      if(!pedestals) pedestals.setSize(data);

      for(size_t x=0; x<data.getSizeX(); ++x){
        for(size_t y=0; y<data.getSizeY(); ++y){
          if(sigmaCut>0){
            const double mean = previous(x,y).getMean();
            const double width = previous(x,y).getSigma()*sigmaCut;
            if(std::fabs(data(x,y)-mean)>width) continue;
          }
          pedestals(x,y).add(data(x,y));
        }
      }
    }
    if(showProgress(eventNr)){
      cout << "Pedestal calculation (" << sigmaCut << " sigma cut): " << eventNr << " events read" << endl;
    }
    ++eventNr;
  }
  swap(pedestalMap,newPedestalMap);
}

int main(int argc, char* argv[])
{
  int skipEvents(0);
  int maxEvents(-1);
  double scaleFactor(1.0);
  vector<string> inputFiles;
  string outputFile;

  int calibrationEvents(2000);
  int calibrationSkip(0);
  double calibrationSigma(5.0);
  vector<string> calibrationFiles;

  //Parse program arguments
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "Show help message")
    ("skip,s", po::value<int>(&skipEvents)->default_value(0),"Number of events to skip before reading")
    ("nevents,n", po::value<int>(&maxEvents)->default_value(-1),"Max. number of output events")
    ("calibrate.nsigma", po::value<double>(&calibrationSigma)->default_value(3.0),"Apply N Sigma cut on adc values when calculating pedestals")
    ("calibrate.nevents", po::value<int>(&calibrationEvents)->default_value(2000),"Total number of events to use for calibration")
    ("calibrate.skip", po::value<int>(&calibrationSkip)->default_value(0),"Number of events to skip in calibration file")
    ("calibration", po::value< vector<string> >(&calibrationFiles),"Datafile to use for calibration, default is to use same as input files")
    ("input,i", po::value< vector<string> >(&inputFiles)->composing(),"Input files")
    ("output,o", po::value<string>(&outputFile)->default_value("output.dat"), "Output file")
    ("no-output", "If present, no output file will be created (except noise and pedestal files if specified)")
    ("noise", po::value<string>(), "Filename to write noise map. If none is given, no noise map is written")
    ("pedestals", po::value<string>(), "Filename to write pedestal map. If none is given, no noise map is written")
    ("scale", po::value<double>(&scaleFactor)->default_value(1.0), "Scaling factor for ADC values")
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
  if(calibrationFiles.empty()) calibrationFiles = inputFiles;

  DEPFET::DataReader reader;
  //Common mode correction: row wise correction using two half rows and one column
  DEPFET::CommonMode commonMode(2,1,2,1);
  map<int, PixelMean> pedestalMap;
  map<int, PixelMean> noiseMap;

  //Calibration: Calculate pedestals, first run: determine mean and sigma for each pixel
  reader.open(calibrationFiles,calibrationEvents);
  reader.skip(calibrationSkip);
  calculatePedestals(reader, pedestalMap, 0);

  //Second run, this time exclude values outside of calibrationSigma * pedestal spread
  reader.open(calibrationFiles,calibrationEvents);
  reader.skip(calibrationSkip);
  calculatePedestals(reader, pedestalMap, calibrationSigma);

  //Write pedestals to file
  if(vm.count("pedestals")>0){
    string pedestalFilename = vm["pedestals"].as<string>();
    ofstream pedestalFile(pedestalFilename.c_str());
    if(!pedestalFile) {
      cerr << "Could not open pedestal output file " << pedestalFilename << endl;
      return 3;
    }
    dumpValues(pedestalFile, pedestalMap, scaleFactor);
    pedestalFile.close();
  }

  //Convert data
  ofstream output;
  if(!vm.count("no-output")>0){
    output.open(outputFile.c_str());
    if(!output){
      cerr << "Could not open output file " << outputFile << endl;
      return 3;
    }
  }

  int eventNr(1);
  reader.open(inputFiles,maxEvents);
  reader.skip(skipEvents);
  while(reader.next()){
    DEPFET::Event &event = reader.getEvent();
    if(output) output << "event " << event.getRunNumber() << " " << event.getEventNumber() << " " << event.size() << endl;
    BOOST_FOREACH(DEPFET::ADCValues &data, event){
      if(output) output << "module " << data.getModuleNr() << " " << data.getSizeX() << " " << data.getSizeY() << endl;
      PixelMean &pedestals = pedestalMap[data.getModuleNr()];
      PixelMean &noise = noiseMap[data.getModuleNr()];
      if(!noise) noise.setSize(data);
      //Pedestal substraction
      data.substract(pedestals);
      //Common Mode correction
      commonMode.apply(data);
      //At this point, data(x,y) is the pixel value of column x, row y
      for(size_t y=0; y<data.getSizeY(); ++y){
        for(size_t x=0; x<data.getSizeX(); ++x){
          dumpValue(output, data(x,y), scaleFactor);
          //Add signal to noise map if it is below nSigma*(sigma of pedestal)
          if(std::fabs(data(x,y))<calibrationSigma*pedestals(x,y).getSigma()) {
            noise(x,y).add(data(x,y));
          }
        }
        if(output) output << endl;
      }
    }
    if(output) output << endl;
    if(showProgress(eventNr)){
      cout << "Output: " << eventNr << " events written" << endl;
    }
    ++eventNr;
  }
  output.close();

  //Write noise to file
  if(vm.count("noise")>0){
    string noiseFilename = vm["noise"].as<string>();
    ofstream noiseFile(noiseFilename.c_str());
    if(!noiseFile) {
      cerr << "Could not open noise output file " << noiseFilename << endl;
      return 3;
    }
    dumpValues(noiseFile, noiseMap, scaleFactor);
    noiseFile.close();
  }
}
