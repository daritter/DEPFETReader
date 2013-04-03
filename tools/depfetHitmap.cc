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

bool showProgress(int event, int minOrder = 0, int maxOrder = 3)
{
  int order = (event == 0) ? 1 : max(min((int)log10(event), maxOrder), minOrder);
  int interval = static_cast<int>(pow(10., order));
  return (event % interval == 0);
}

typedef DEPFET::ValueMatrix<double> PixelValues;

int main(int argc, char* argv[])
{
  int skipEvents(0);
  int maxEvents(-1);
  vector<string> inputFiles;
  string outputFile;
  string calibrationFile;
  double sigmaCut(5.0);
  bool do_normalize(false);
  int frameNr(-1);

  //Parse program arguments
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", "Show help message")
  ("skip,s", po::value<int>(&skipEvents)->default_value(0), "Number of events to skip before reading")
  ("sigma", po::value<double>(&sigmaCut)->default_value(5.0), "Sigma cut to apply to data")
  ("nevents,n", po::value<int>(&maxEvents)->default_value(-1), "Max. number of output events")
  ("calibration,c", po::value<string>(&calibrationFile), "Calibration File")
  ("input,i", po::value< vector<string> >(&inputFiles)->composing(), "Input files")
  ("output,o", po::value<string>(&outputFile)->default_value("hitmap.dat"), "Output file")
  ("4fold", "If set, data is read out in 4fold mode, otherwise 2fold")
  ("dcd", "If set, common mode corretion is set to DCD mode (4 full rows), otherwise curo topology is used (two half rows")
  ("trailing", po::value<int>(), "Set number of trailing frames")
  ("normalize", po::bool_switch(), "Divide ADC count by number of frames processed")
  ("onlyFrame", po::value<int>(&frameNr), "Set the frame number to be used: -1=all, 0=original, 1=1st tailing, ...")
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
    commonMode = DEPFET::CommonMode(4, 0, 1, 1);
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

  while (calStream) {
    int col, row, px_masked;
    double px_pedestal, px_noise;
    calStream >> col >> row >> px_masked >> px_pedestal >> px_noise;
    if (!calStream) break;
    mask.at(col, row) = px_masked;
    pedestals.at(col, row) = px_pedestal;
    noise.at(col, row) = px_noise;
  }

  hitmap.substract(mask, 1e4);
  commonMode.setMask(&mask);
  commonMode.setNoise(sigmaCut, &noise);

  int eventNr(1);
  reader.open(inputFiles, maxEvents);
  reader.skip(skipEvents);
  while (reader.next()) {
    DEPFET::Event& event = reader.getEvent();
    BOOST_FOREACH(DEPFET::ADCValues & data, event) {
      if (frameNr >= 0 && data.getFrameNr() != frameNr) continue;
      // DEPFET::ADCValues &data = event[0];
      //Pedestal substraction
      data.substract(pedestals);
      //Common Mode correction
      commonMode.apply(data);
      //At this point, data(x,y) is the pixel value of column x, row y
      for (size_t y = 0; y < data.getSizeY(); ++y) {
        //Mask startgate
        //if(y%2 == data.getStartGate()) {
        //continue;
        //}
        for (size_t x = 0; x < data.getSizeX(); ++x) {
          if (data(x, y) > sigmaCut * noise(x, y)) {
            hitmap(x, y) += data(x, y);
          }
        }
      }
    }
    if (showProgress(eventNr)) {
      cout << "Output: " << eventNr << " events written" << endl;
    }
    ++eventNr;
  }

  ofstream hitmapFile(outputFile.c_str());
  if (!hitmapFile) {
    cerr << "Could not open hitmap output file " << outputFile;
  }
  hitmapFile << hitmap.getSizeX() << " " << hitmap.getSizeY() << endl;
  for (unsigned int col = 0; col < hitmap.getSizeX(); ++col) {
    for (unsigned int row = 0; row < hitmap.getSizeY(); ++row) {
      //Normalize
      if (do_normalize) hitmap(col, row) /= (eventNr - 1);
      hitmapFile << hitmap(col, row) << " ";
    }
    hitmapFile << endl;
  }
  hitmapFile.close();
}
