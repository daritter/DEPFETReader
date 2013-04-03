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

//Output a single value to file
inline void dumpValue(ostream& output, double value, double scale = 1.0)
{
  if (isnan(value)) value = 0;
  if (output) output << setprecision(2) << setw(8) << fixed << (value * scale) << " ";
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
  ("output,o", po::value<string>(&outputFile)->default_value("data.dat"), "Output file")
  ("4fold", "If set, data is read out in 4fold mode, otherwise 2fold")
  ("dcd", "If set, common mode corretion is set to DCD mode (4 full rows), otherwise curo topology is used (two half rows")
  ("frame,f", po::value<int>(&frameNr)->default_value(frameNr), "Set the frame number to be used: -1=all, 0=original, 1=1st tailing, ...")
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

  ofstream output(outputFile.c_str());
  if (!output) {
    cerr << "Could not open output file" << endl;
    return 3;
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

  //Read depfet calibration from file
  DEPFET::PixelMask mask;
  PixelValues pedestals;
  PixelValues noise;

  DEPFET::Event& event = reader.getEvent();
  mask.setSize(event[0]);
  pedestals.setSize(mask);
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

  commonMode.setMask(&mask);
  commonMode.setNoise(sigmaCut, &noise);

  //Done reading calibration, now read the events

  int eventNr(1);
  reader.open(inputFiles, maxEvents);
  reader.skip(skipEvents);
  while (reader.next()) {
    DEPFET::Event& event = reader.getEvent();
    output << "event " << event.getRunNumber() << " " << event.getEventNumber() << " " << event.size() << endl;
    BOOST_FOREACH(DEPFET::ADCValues & data, event) {
      if (frameNr >= 0 && data.getFrameNr() != frameNr) continue;
      output << "module " << data.getModuleNr() << " " << data.getSizeX() << " " << data.getSizeY() << endl;
      //Pedestal substraction
      data.substract(pedestals);
      //Common Mode correction
      commonMode.apply(data);
      //At this point, data(x,y) is the pixel value of column x, row y
      //Insert custom code here --->
      for (size_t y = 0; y < data.getSizeY(); ++y) {
        //Mask startgate
        for (size_t x = 0; x < data.getSizeX(); ++x) {
          double adc = data(x, y);
          if (adc < noise(x, y)*sigmaCut) adc = 0;
          //if(y%2 == data.getStartGate()) adc = 0;
          if (mask(x, y)) adc = -1;
          dumpValue(output, adc);
        }
        output << endl;
      }
      //---> Done
    }
    if (output) output << endl;
    if (showProgress(eventNr)) {
      cout << "Output: " << eventNr << " events written" << endl;
    }
    ++eventNr;
  }

  //Close the output file
  output.close();
}
