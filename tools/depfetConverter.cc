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

bool showProgress(int event, int minOrder = 1, int maxOrder = 3)
{
  int order = (event == 0) ? 1 : max(min((int)log10(event), maxOrder), minOrder);
  int interval = static_cast<int>(pow(10., order));
  return (event % interval == 0);
}

typedef DEPFET::ValueMatrix<DEPFET::IncrementalMean> PixelMean;
typedef map<int, PixelMean> PixelMeanMap;
typedef map<int, DEPFET::PixelNoise> PixelNoiseMap;
typedef map<int, DEPFET::PixelMask> PixelMaskMap;

//Output a single value to file
inline void dumpValue(ostream& output, double value, double scale)
{
  if (isnan(value)) value = 0;
  if (output) output << setprecision(2) << setw(8) << fixed << (value * scale) << " ";
}

//Output a matrix of mean/sigma values for each module
void dumpValues(ostream& output, const PixelMeanMap& dataMap, double scaleFactor)
{
  output << "mean " << dataMap.size() << endl;
  BOOST_FOREACH(const PixelMeanMap::value_type & it, dataMap) {
    const PixelMean& data = it.second;
    output << "module " << it.first << " " << data.getSizeX() << " " << data.getSizeY() << endl;
    for (size_t y = 0; y < data.getSizeY(); ++y) {
      for (size_t x = 0; x < data.getSizeX(); ++x) {
        dumpValue(output, data(x, y).getMean(), scaleFactor);
      }
      output << endl;
    }
  }
  output << endl;

  output << "sigma " << dataMap.size() << endl;
  BOOST_FOREACH(const PixelMeanMap::value_type & it, dataMap) {
    const PixelMean& data = it.second;
    output << "module " << it.first << " " << data.getSizeX() << " " << data.getSizeY() << endl;
    for (size_t y = 0; y < data.getSizeY(); ++y) {
      for (size_t x = 0; x < data.getSizeX(); ++x) {
        dumpValue(output, data(x, y).getSigma(), scaleFactor);
      }
      output << endl;
    }
  }
  output << endl;
}

//Calculate the pedestals: Determine mean and sigma of every pixel, optionally
//applying a cut using mean and sigma of a previous run
void calculatePedestals(DEPFET::DataReader& reader, PixelMeanMap& pedestalMap, double sigmaCut, PixelMaskMap& maskedMap)
{
  PixelMeanMap newPedestalMap;
  int eventNr(1);
  while (reader.next()) {
    DEPFET::Event& event = reader.getEvent();
    BOOST_FOREACH(DEPFET::ADCValues & data, event) {
      const PixelMean& previous = pedestalMap[data.getModuleNr()];
      PixelMean& pedestals = newPedestalMap[data.getModuleNr()];
      const DEPFET::PixelMask& masked = maskedMap[data.getModuleNr()];
      if (!pedestals) pedestals.setSize(data);

      for (size_t x = 0; x < data.getSizeX(); ++x) {
        for (size_t y = 0; y < data.getSizeY(); ++y) {
          if (masked(x, y)) continue;
          if (sigmaCut > 0) {
            const double mean = previous(x, y).getMean();
            const double width = previous(x, y).getSigma() * sigmaCut;
            if (std::fabs(data(x, y) - mean) > width) continue;
          }
          pedestals(x, y).add(data(x, y));
        }
      }
    }
    if (showProgress(eventNr)) {
      cout << "Pedestal calculation (" << sigmaCut << " sigma cut): " << eventNr << " events read" << endl;
    }
    ++eventNr;
  }
  swap(pedestalMap, newPedestalMap);
}

int main(int argc, char* argv[])
{
  int skipEvents(0);
  int maxEvents(-1);
  double scaleFactor(1.0);
  vector<string> inputFiles;
  string outputFile;
  string maskFile("MaskCh-Mod%1%.txt");

  int calibrationEvents(2000);
  int calibrationSkip(0);
  double calibrationSigma(5.0);
  vector<string> calibrationFiles;

  //Parse program arguments
  po::options_description desc("Allowed options");
  desc.add_options()
  ("help,h", "Show help message")
  ("skip,s", po::value<int>(&skipEvents)->default_value(0), "Number of events to skip before reading")
  ("nevents,n", po::value<int>(&maxEvents)->default_value(-1), "Max. number of output events")
  ("mask", po::value<string>(&maskFile), "Filename for reading pixel masks")
  ("calibrate.nsigma", po::value<double>(&calibrationSigma)->default_value(3.0), "Apply N Sigma cut on adc values when calculating pedestals")
  ("calibrate.nevents", po::value<int>(&calibrationEvents)->default_value(2000), "Total number of events to use for calibration")
  ("calibrate.skip", po::value<int>(&calibrationSkip)->default_value(0), "Number of events to skip in calibration file")
  ("calibration", po::value< vector<string> >(&calibrationFiles), "Datafile to use for calibration, default is to use same as input files")
  ("input,i", po::value< vector<string> >(&inputFiles)->composing(), "Input files")
  ("output,o", po::value<string>(&outputFile)->default_value("output.dat"), "Output file")
  ("no-output", "If present, no output file will be created (except noise and pedestal files if specified)")
  ("noise", po::value<string>(), "Filename to write noise map. If none is given, no noise map is written")
  ("pedestals", po::value<string>(), "Filename to write pedestal map. If none is given, no noise map is written")
  ("combined", po::value<string>(), "Filename to write combined pedestal/mask map containing col, row, masked, pedestal, noise")
  ("hitmap", po::value<string>(), "Filename for the hitmap output")
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
  if (inputFiles.empty()) {
    cerr << "No input files given" << endl;
    return 2;
  }
  if (calibrationFiles.empty()) calibrationFiles = inputFiles;

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

  PixelMeanMap pedestalMap;
  PixelMeanMap noiseMap;
  PixelMaskMap maskedMap;

  reader.open(inputFiles, maxEvents);
  if (!reader.next()) {
    cerr << "Could not read a single event from the file" << cerr;
    return 5;
  }
  DEPFET::Event& event = reader.getEvent();
  boost::format maskFileFormat(maskFile);
  BOOST_FOREACH(DEPFET::ADCValues & data, event) {
    //pedestalMap[data.getModuleNr()].setSize(data);
    noiseMap[data.getModuleNr()].setSize(data);
    DEPFET::PixelMask& pixelmask = maskedMap[data.getModuleNr()];
    pixelmask.setSize(data);
    if (!maskFile.empty()) {
      ifstream maskStream((maskFileFormat % data.getModuleNr()).str().c_str());
      if (!maskStream) {
        cerr << "Could not open mask file for module " << data.getModuleNr() << ", not masking any pixels" << endl;
      }
      //Skip first line
      if (maskStream.peek() == '*') {
        maskStream.ignore(1000, '\n');
      }
      //Use the rest for masking
      while (maskStream) {
        int col, row;
        maskStream >> col >> row;
        if (!maskStream) break;
        if (col < 0) for (col = 0; col < (int)pixelmask.getSizeX(); ++col) {
            pixelmask(col, row) = 1;
          } else  if (row < 0) for (row = 0; row < (int)pixelmask.getSizeY(); ++row) {
            pixelmask(col, row) = 1;
          } else
          pixelmask(col, row) = 1;
      }
    }
  }

  //Calibration: Calculate pedestals, first run: determine mean and sigma for each pixel
  reader.open(calibrationFiles, calibrationEvents);
  reader.skip(calibrationSkip);
  calculatePedestals(reader, pedestalMap, 0, maskedMap);

  //Second run, this time exclude values outside of calibrationSigma * pedestal spread
  reader.open(calibrationFiles, calibrationEvents);
  reader.skip(calibrationSkip);
  calculatePedestals(reader, pedestalMap, calibrationSigma, maskedMap);

  {
    //Third run to determine noise level of pixels
    reader.open(calibrationFiles, calibrationEvents);
    reader.skip(calibrationSkip);
    int eventNr(1);
    while (reader.next()) {
      DEPFET::Event& event = reader.getEvent();
      BOOST_FOREACH(DEPFET::ADCValues & data, event) {
        const PixelMean& pedestals = pedestalMap[data.getModuleNr()];
        PixelMean& noise = noiseMap[data.getModuleNr()];
        const DEPFET::PixelMask& masked = maskedMap[data.getModuleNr()];
        commonMode.setMask(&masked);
        //Pedestal substraction
        data.substract(pedestals);
        //Common Mode correction
        commonMode.apply(data);
        for (size_t x = 0; x < data.getSizeX(); ++x) {
          for (size_t y = 0; y < data.getSizeY(); ++y) {
            double signal = data(x, y);
            //Add signal to noise map if it is below nSigma*(sigma of pedestal)
            if (std::fabs(signal) > calibrationSigma * pedestals(x, y).getSigma()) continue;
            noise(x, y).add(signal);
          }
        }
      }
      if (showProgress(eventNr)) {
        cout << "Calculating Noise: " << eventNr << " events read" << endl;
      }
      ++eventNr;
    }
  }

  //Write pedestals to file
  if (vm.count("pedestals") > 0) {
    string pedestalFilename = vm["pedestals"].as<string>();
    ofstream pedestalFile(pedestalFilename.c_str());
    if (!pedestalFile) {
      cerr << "Could not open pedestal output file " << pedestalFilename << endl;
      return 3;
    }
    dumpValues(pedestalFile, pedestalMap, scaleFactor);
    pedestalFile.close();
  }

  //Write noise to file
  if (vm.count("noise") > 0) {
    string noiseFilename = vm["noise"].as<string>();
    ofstream noiseFile(noiseFilename.c_str());
    if (!noiseFile) {
      cerr << "Could not open noise output file " << noiseFilename << endl;
      return 3;
    }
    dumpValues(noiseFile, noiseMap, scaleFactor);
    noiseFile.close();
  }

  //Write combined calibration file
  if (vm.count("combined") > 0) {
    boost::format combinedFormat(vm["combined"].as<string>());
    combinedFormat.exceptions(boost::io::all_error_bits ^ (boost::io::too_many_args_bit));
    BOOST_FOREACH(PixelMaskMap::value_type & mask, maskedMap) {
      string combinedFilename = (combinedFormat % mask.first).str();
      ofstream combinedFile(combinedFilename.c_str());
      if (!combinedFile) {
        cerr << "Could not open combined output file " << combinedFilename;
      }
      const PixelMean& pedestals = pedestalMap[mask.first];
      const PixelMean& noise = noiseMap[mask.first];
      for (unsigned int col = 0; col < pedestals.getSizeX(); ++col) {
        for (unsigned int row = 0; row < pedestals.getSizeY(); ++row) {
          combinedFile << setw(6) << col << setw(6) << row << setw(2) << (int)mask.second(col, row) << " ";
          dumpValue(combinedFile, pedestals(col, row).getMean(), scaleFactor);
          dumpValue(combinedFile, noise(col, row).getSigma(), scaleFactor);
          combinedFile << endl;
        }
      }

    }
  }

  PixelNoiseMap finalNoiseMap;
  BOOST_FOREACH(const PixelMeanMap::value_type & noise, noiseMap) {
    DEPFET::PixelNoise& n = finalNoiseMap[noise.first];
    n.setSize(noise.second);
    for (unsigned int i = 0; i < noise.second.getSize(); ++i) {
      n[i] = noise.second[i].getSigma();
    }
  }

  //Convert data
  ofstream output;
  if (!vm.count("no-output") > 0) {
    output.open(outputFile.c_str());
    if (!output) {
      cerr << "Could not open output file " << outputFile << endl;
      return 3;
    }
    //}else{
    //return 0;
  }

  int eventNr(1);
  reader.open(inputFiles, maxEvents);
  reader.skip(skipEvents);
  PixelNoiseMap hitmapMap;
  while (reader.next()) {
    DEPFET::Event& event = reader.getEvent();
    if (output) output << "event " << event.getRunNumber() << " " << event.getEventNumber() << " " << event.size() << endl;
    BOOST_FOREACH(DEPFET::ADCValues & data, event) {
      if (output) output << "module " << data.getModuleNr() << " " << data.getSizeX() << " " << data.getSizeY() << endl;
      const PixelMean& pedestals = pedestalMap[data.getModuleNr()];
      const DEPFET::PixelNoise& noise = finalNoiseMap[data.getModuleNr()];
      const DEPFET::PixelMask& masked = maskedMap[data.getModuleNr()];
      //Pedestal substraction
      data.substract(pedestals);
      //Common Mode correction
      commonMode.setMask(&masked);
      commonMode.setNoise(5, &noise);
      commonMode.apply(data);
      DEPFET::PixelNoise& hitmap = hitmapMap[data.getModuleNr()];
      if (eventNr == 1) {
        hitmap.setSize(data);
        hitmap.substract(masked, 1e4);
      }
      //At this point, data(x,y) is the pixel value of column x, row y
      for (size_t y = 0; y < data.getSizeY(); ++y) {
        for (size_t x = 0; x < data.getSizeX(); ++x) {
          dumpValue(output, data(x, y), scaleFactor);
          if (data(x, y) > 5 * noise(x, y)) {
            hitmap(x, y) += data(x, y);
          }
        }
        if (output) output << endl;
      }
    }
    if (output) output << endl;
    if (showProgress(eventNr)) {
      cout << "Output: " << eventNr << " events written" << endl;
    }
    ++eventNr;
  }
  if (output) output.close();

  if (vm.count("hitmap") > 0) {
    boost::format hitmapFormat(vm["hitmap"].as<string>());
    hitmapFormat.exceptions(boost::io::all_error_bits ^ (boost::io::too_many_args_bit));
    BOOST_FOREACH(PixelNoiseMap::value_type & hitmapit, hitmapMap) {
      string hitmapFilename = (hitmapFormat % hitmapit.first).str();
      ofstream hitmapFile(hitmapFilename.c_str());
      if (!hitmapFile) {
        cerr << "Could not open hitmap output file " << hitmapFilename;
      }
      DEPFET::PixelNoise& hitmap = hitmapit.second;
      hitmapFile << hitmap.getSizeX() << " " << hitmap.getSizeY() << endl;
      for (unsigned int col = 0; col < hitmap.getSizeX(); ++col) {
        for (unsigned int row = 0; row < hitmap.getSizeY(); ++row) {
          hitmapFile << hitmap(col, row) << " ";
        }
        hitmapFile << endl;
      }
    }
  }
}
