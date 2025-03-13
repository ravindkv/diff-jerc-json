#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include "SkimTree.h"
#include "Helper.h"

SkimTree::SkimTree(GlobalFlag& globalFlags): 
    globalFlags_(globalFlags),
    year_(globalFlags_.getYear()),
    era_(globalFlags_.getEra()),
    channel_(globalFlags_.getChannel()),
    isDebug_(globalFlags_.isDebug()),
    isData_(globalFlags_.isData()),
    isMC_(globalFlags_.isMC()),
	fCurrent_(-1), 
	outName_(""), 
	fChain_(std::make_unique<TChain>("Events")) {
    std::cout << "+ SkimTree initialized with outName = " << outName_ << '\n';
}

SkimTree::~SkimTree() {
    // unique_ptr will automatically delete fChain_
}

void SkimTree::setInput(const std::string& outName) {
    outName_ = outName;
    std::cout << "+ setInput() = " << outName_ << '\n';
}

void SkimTree::loadInput() {
    std::cout << "==> loadInput()" << '\n';
    try {
        std::vector<std::string> v_outName = Helper::splitString(outName_, "_Hist_");
        if (v_outName.size() < 2) {
            throw std::runtime_error("Invalid outName format: Expected at least two parts separated by '_Hist_'");
        }
        loadedSampKey_ = v_outName.at(0);
        std::cout << "loadedSampKey_: " << loadedSampKey_ << '\n';

        std::string nofN_root = v_outName.at(1);
        std::vector<std::string> v_nofN_root = Helper::splitString(nofN_root, ".root");
        if (v_nofN_root.empty()) {
            throw std::runtime_error("Invalid outName format: Missing '.root' extension");
        }

        std::string nofN = v_nofN_root.at(0);
        std::cout << "nofN: " << nofN << '\n';

        std::vector<std::string> v_nofN = Helper::splitString(nofN, "of");
        if (v_nofN.size() != 2) {
            throw std::runtime_error("Invalid job numbering format in outName: Expected format 'NofM'");
        }

        loadedNthJob_ = std::stoi(v_nofN.at(0));
        loadedTotJob_ = std::stoi(v_nofN.at(1));
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Error in loadInput(): " << e.what() << "\n"
            << "Check the outName_: " << outName_ << "\n"
            << "outName format should be: DataOrMC_Year_Channel_Sample_Hist_NofM.root\n"
            << "Run ./runMain -h for more details";
        throw std::runtime_error(oss.str());
    }
}

void SkimTree::setInputJsonPath(const std::string& inDir) {
    std::string year;
    if (year_ == GlobalFlag::Year::Year2016Pre)
        year = "2016Pre";
    else if (year_ == GlobalFlag::Year::Year2016Post)
        year = "2016Post";
    else if (year_ == GlobalFlag::Year::Year2017)
        year = "2017";
    else if (year_ == GlobalFlag::Year::Year2018)
        year = "2018";
    else if (year_ == GlobalFlag::Year::Year2024)
        year = "2024";
    else {
        throw std::runtime_error("Error: Provide correct year in SkimTree::setInputJsonPath()");
    }

    std::vector<std::string> tokens = Helper::splitString(loadedSampKey_, "_");
    if (tokens.size() < 3) {
        throw std::runtime_error("Invalid loadedSampKey_ format: Expected at least three parts separated by '_'");
    }
    std::string channel = tokens.at(1);
    inputJsonPath_ = inDir + "/FilesNano_" + channel + "_" + year + ".json";
    std::cout << "+ setInputJsonPath() = " << inputJsonPath_ << '\n';
}

void SkimTree::loadInputJson() {
    std::cout << "==> loadInputJson()" << '\n';
    std::ifstream fileName(inputJsonPath_);
    if (!fileName.is_open()) {
        throw std::runtime_error("Unable to open input JSON file: " + inputJsonPath_);
    }

    nlohmann::json js;
    try {
        fileName >> js;
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Error parsing input JSON file: " << inputJsonPath_ << "\n"
            << e.what();
        throw std::runtime_error(oss.str());
    }

    try {
        js.at(loadedSampKey_).get_to(loadedAllFileNames_);
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Key not found in JSON: " << loadedSampKey_ << "\n"
            << e.what() << "\n"
            << "Available keys in the JSON file:";
        for (const auto& element : js.items()) {
            oss << "\n- " << element.key();
        }
        throw std::runtime_error(oss.str());
    }
}

void SkimTree::loadJobFileNames() {
    std::cout << "==> loadJobFileNames()" << '\n';
    int nFiles = static_cast<int>(loadedAllFileNames_.size());
    std::cout << "Total files = " << nFiles << '\n';

    if (loadedTotJob_ > nFiles) {
        std::cout << "Since loadedTotJob_ > nFiles, setting loadedTotJob_ to nFiles: " << nFiles << '\n';
        loadedTotJob_ = nFiles;
    }

    if (loadedNthJob_ > loadedTotJob_) {
        throw std::runtime_error("Error: loadedNthJob_ > loadedTotJob_ in loadJobFileNames()");
    }

    if (loadedNthJob_ > 0 && loadedTotJob_ > 0) {
        std::cout << "Jobs: " << loadedNthJob_ << " of " << loadedTotJob_ << '\n';
        std::cout << static_cast<double>(nFiles) / loadedTotJob_ << " files per job on average" << '\n';
    } else {
        throw std::runtime_error("Error: Make sure loadedNthJob_ > 0 and loadedTotJob_ > 0 in loadJobFileNames()");
    }

    std::vector<std::vector<std::string>> smallVectors = Helper::splitVector(loadedAllFileNames_, loadedTotJob_);
    if (loadedNthJob_ - 1 >= static_cast<int>(smallVectors.size())) {
        throw std::runtime_error("Error: loadedNthJob_ is out of range after splitting file names in loadJobFileNames()");
    }
    loadedJobFileNames_ = smallVectors[loadedNthJob_ - 1];
}

void SkimTree::loadTree() {
    std::cout << "==> loadTree()" << '\n';
    if (!fChain_) {
        fChain_ = std::make_unique<TChain>("Events");
    }
    fChain_->SetCacheSize(100 * 1024 * 1024);

    if (loadedJobFileNames_.empty()) {
        throw std::runtime_error("Error: No files to load in loadTree()");
    }

    bool isCopy = false;  // Set to true if you want to copy files locally
    std::string dir = "root://cms-xrd-global.cern.ch/";  // Default remote directory

    int totalFiles = 0;
    int addedFiles = 0;
    int failedFiles = 0;

    // Optimization parameters for xrdcp
    const int streams = 15;              // Number of parallel data streams
    const int tcpBufSize = 1048576;      // TCP buffer size (1MB)

    for (const auto& fileName : loadedJobFileNames_) {
        totalFiles++;
        std::string fullPath;

        if (isCopy) {
            // Extract the local file name from the remote path
            std::string localFile = fileName.substr(fileName.find_last_of('/') + 1);
            std::string cmd = "xrdcp ";

            // Append optimization options to the xrdcp command
            cmd += "--streams " + std::to_string(streams) + " ";

            // Construct the full remote path
            std::string remoteFile = dir + fileName;

            // Build the final command
            cmd += remoteFile + " " + localFile;

            std::cout << "Executing command: " << cmd << '\n';
            int ret = system(cmd.c_str());

            if (ret != 0) {
                std::cerr << "Error: Failed to copy " << remoteFile << " to local file " << localFile << '\n';
                failedFiles++;
                continue;  // Skip adding this file
            }

            // Check if the file was successfully copied
            if (!std::filesystem::exists(localFile)) {
                std::cerr << "Error: Local file " << localFile << " does not exist after copying.\n";
                failedFiles++;
                continue;  // Skip adding this file
            }

            fullPath = localFile;  // Use the local file path
        } else {
            // Remote file handling
            std::filesystem::path filePath = "/eos/cms/" + fileName;
            if (std::filesystem::exists(filePath)) {
                dir = "/eos/cms/";  // Use local EOS path
                fullPath = dir + fileName;
            } else {
                dir = "root://cms-xrd-global.cern.ch/";  // Fallback to remote
                fullPath = dir + fileName;
            }
        }

        // Attempt to open the file to verify its validity
        TFile* f = TFile::Open(fullPath.c_str(), "READ");
        if (!f || f->IsZombie()) {
            std::cerr << "Error: Failed to open or corrupted file " << fullPath << '\n';
            if (f) f->Close();
            failedFiles++;
            continue;  // Skip adding this file
        }

        // Check if "Events" tree exists
        if (!f->GetListOfKeys()->Contains("Events")) {
            std::cerr << "Error: 'Events' not found in " << fullPath << '\n';
            f->Close();
            failedFiles++;
            continue;  // Skip adding this file
        }

        // Check the entries in the newly added TTree
        Long64_t fileEntries = f->Get<TTree>("Events")->GetEntries();
        if (fileEntries == 0) {
            std::cerr << "\nWarning: 'Events' TTree in file " << fullPath << " has 0 entries. Skipping file.\n\n";
            f->Close();
            failedFiles++;
            continue;  // Skip adding this file to the final count
        }

        // File is valid, add it to the TChain
        int added = fChain_->Add(fullPath.c_str());
        if (added == 0) {
            std::cerr << "Warning: TChain::Add failed for " << fullPath << '\n';
            f->Close();
            failedFiles++;
            continue;  // Skip adding this file
        }

        std::cout << fullPath << "  Entries: " << fChain_->GetEntries() << '\n';
        addedFiles++;
        f->Close();
    }

    fChain_->SetBranchStatus("*", false);
    fChain_->SetBranchStatus("run", true);
    fChain_->SetBranchStatus("luminosityBlock", true);
    fChain_->SetBranchStatus("event", true);

    fChain_->SetBranchAddress("run", &run);
    fChain_->SetBranchAddress("luminosityBlock", &luminosityBlock);
    fChain_->SetBranchAddress("event", &event);

	//--------------------------------------- 
	//Jet for all channels 
	//--------------------------------------- 
	fChain_->SetBranchStatus("nJet", true); 
	fChain_->SetBranchStatus("Jet_area", true); 
	fChain_->SetBranchStatus("Jet_eta"     , true); 
	fChain_->SetBranchStatus("Jet_mass"    , true); 
	fChain_->SetBranchStatus("Jet_phi"     , true); 
	fChain_->SetBranchStatus("Jet_pt"    , true); 
	fChain_->SetBranchStatus("Jet_rawFactor", true); 
	fChain_->SetBranchStatus("Jet_jetId", true); 

	fChain_->SetBranchAddress("nJet", &nJet);
	fChain_->SetBranchAddress("Jet_area", &Jet_area);
	fChain_->SetBranchAddress("Jet_eta"     , &Jet_eta);
	fChain_->SetBranchAddress("Jet_mass"    , &Jet_mass);
	fChain_->SetBranchAddress("Jet_phi"     , &Jet_phi);
	fChain_->SetBranchAddress("Jet_pt"    , &Jet_pt);
	fChain_->SetBranchAddress("Jet_rawFactor", &Jet_rawFactor);
	fChain_->SetBranchAddress("Jet_jetId", &Jet_jetId);
}

auto SkimTree::getEntries() const -> Long64_t {
    return fChain_ ? fChain_->GetEntries() : 0;
}

auto SkimTree::getChain() const -> TChain* {
    return fChain_.get();  // Return raw pointer to fChain_
}

auto SkimTree::getEntry(Long64_t entry) -> Int_t {
    return fChain_ ? fChain_->GetEntry(entry) : 0;
}

auto SkimTree::loadEntry(Long64_t entry) -> Long64_t {
    // Set the environment to read one entry
    if (!fChain_) {
        throw std::runtime_error("Error: fChain_ is not initialized in loadEntry()");
    }
    Long64_t centry = fChain_->LoadTree(entry);
    if (centry < 0) {
        throw std::runtime_error("Error loading entry in loadEntry()");
    }
    if (fChain_->GetTreeNumber() != fCurrent_) {
        fCurrent_ = fChain_->GetTreeNumber();
    }
    // Uncomment for debugging
    // std::cout << entry << ", " << centry << ", " << fCurrent_ << std::endl;
    return centry;
}

