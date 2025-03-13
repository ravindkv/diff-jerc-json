#include "RunZeeJet.h"
#include "SkimTree.h"
#include "ScaleObject.h"
#include "GlobalFlag.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>

#include <memory> // Include for unique_ptr

using namespace std;
namespace fs = std::filesystem;


int main(int argc, char* argv[]) {
  // Check if no arguments are provided
  if (argc == 1) {
    std::cerr << "Error: No arguments provided. Use -h for help." << std::endl;
    return 1;
  }
  const std::string& metadataJsonPath = "input/jerc/metadata.json";


  std::string jsonDir = "input/root/json/";
  std::vector<std::string> jsonFiles;

  // Read only FilesNano_*.json files in the directory
  for (const auto& entry : fs::directory_iterator(jsonDir)) {
    if (entry.path().extension() == ".json" &&
        entry.path().filename().string().rfind("FilesNano_", 0) == 0) {
      jsonFiles.push_back(entry.path().string());
    }
  }

  if (jsonFiles.empty()) {
    std::cerr << "No JSON files found in directory: " << jsonDir << std::endl;
    return 1;
  }

  nlohmann::json js;
  std::string outName;

  //--------------------------------
  // Parse command-line options
  //--------------------------------
  int opt;
  while ((opt = getopt(argc, argv, "o:h")) != -1) {
    switch (opt) {
      case 'o':
        outName = optarg;
        break;
      case 'h':
        // Loop through each JSON file and print available keys
        for (const auto& jsonFile : jsonFiles) {
          std::ifstream file(jsonFile);
          if (!file.is_open()) {
            std::cerr << "Could not open file: " << jsonFile << std::endl;
            continue;
          }

          try {
            js = nlohmann::json::parse(file);
          } catch (const std::exception& e) {
            std::cerr << "EXCEPTION: Error parsing file: " << jsonFile << std::endl;
            std::cerr << e.what() << std::endl;
            continue;
          }

          std::cout << "\nFor file: " << jsonFile << std::endl;
          for (auto& element : js.items()) {
            std::cout << "./runMain -o " << element.key() << "_Hist_1of100.root" << std::endl;
          }
        }
        return 0;
      default:
        std::cerr << "Use -h for help" << std::endl;
        return 1;
    }
  }

	std::cout << "\n--------------------------------------" << std::endl;
    std::cout << " Set GlobalFlag.cpp" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    
    // Initialize GlobalFlag instance
    GlobalFlag globalFlag(outName);
    globalFlag.setDebug(false);
    globalFlag.setNDebug(1000);
    globalFlag.printFlags();  

    std::cout << "\n--------------------------------------" << std::endl;
    std::cout << " Set and load SkimTree.cpp" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    
    std::shared_ptr<SkimTree> skimT = std::make_shared<SkimTree>(globalFlag);
    skimT->setInput(outName);
    skimT->loadInput();
    skimT->setInputJsonPath(jsonDir);
    skimT->loadInputJson();
    skimT->loadJobFileNames();
    skimT->loadTree();

    std::cout << "\n--------------------------------------" << std::endl;
    std::cout << " Set and load ScaleObject.cpp" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    
    // Pass GlobalFlag reference to ScaleObject
    std::shared_ptr<ScaleObject> scaleObj = std::make_shared<ScaleObject>(globalFlag);
    
    try {
        //scaleObj->setInputs();
        //scaleObj->loadJetVetoRef();
        //scaleObj->loadJetL1FastJetRef();
        //scaleObj->loadJetL2RelativeRef();
        //scaleObj->loadJetL2L3ResidualRef();
        //scaleObj->loadJerResoRef();
        //scaleObj->loadJerSfRef();
       // scaleObj->loadPuRef();

        // Use the GlobalFlag instance for conditional checks
        if (globalFlag.getChannel() == GlobalFlag::Channel::GamJet) {  // Scale and Smearing
            //scaleObj->loadPhoSsRef();
            //scaleObj->loadEleSsRef();
        }
        if (globalFlag.getChannel() == GlobalFlag::Channel::ZmmJet) { 
            //scaleObj->loadMuRochRef();
        }
        if (globalFlag.isData()) {
            //scaleObj->loadLumiJson();
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Critical error: " << e.what() << std::endl;
        return EXIT_FAILURE;  // Exit with failure code
    }

    // Output directory setup
    std::string outDir = "output";
    mkdir(outDir.c_str(), S_IRWXU);
    auto fout = std::make_unique<TFile>((outDir + "/" + outName).c_str(), "RECREATE");

    std::cout << "\n--------------------------------------" << std::endl;
    std::cout << " Loop over events and fill Histos" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    
    if (globalFlag.getChannel() == GlobalFlag::Channel::ZeeJet) {
        std::cout << "==> Running ZeeJet" << std::endl;
        auto zeeJet = std::make_unique<RunZeeJet>(globalFlag);
        zeeJet->Run(skimT, scaleObj.get(), metadataJsonPath, fout.get());
    }
  return 0;
}

