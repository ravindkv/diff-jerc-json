#include "HistGivenBoth.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

#include "TDirectory.h"
#include "TROOT.h"

HistGivenBoth::HistGivenBoth(TDirectory *origDir, const std::string& directoryName,const std::string& metadataJsonPath)
{
    initialize(origDir, directoryName, metadataJsonPath);
}

HistGivenBoth::~HistGivenBoth() {
}

void HistGivenBoth::initialize(TDirectory *origDir, const std::string& directoryName, const std::string& metadataJsonPath){
    // Read metadata JSON
    std::ifstream inFile(metadataJsonPath);
    if (!inFile.is_open()) {
        throw std::runtime_error("HistGivenBoth::initialize: Unable to open metadata JSON: " + metadataJsonPath_);
    }

    nlohmann::json meta;
    inFile >> meta;
    inFile.close();

    // meta has structure:
    // {
    //   "DATA_L1FastJet_AK4PFPuppi": [
    //       ["Winter24Prompt24_V1.json", "Winter24Prompt24_RunBCD_V1_DATA_L1FastJet_AK4PFPuppi"],
    //       ["Winter24Prompt24_V2.json", "Winter24Prompt24_RunBCD_V2_DATA_L1FastJet_AK4PFPuppi"]
    //   ],
    //   "DATA_L2Relative_AK4PFPuppi": [...]
    //   ...
    // }

    // Use the Helper method to get or create the directory
    std::string dirName = "HistGivenBoth/"+directoryName;
    TDirectory* newDir = Helper::createTDirectory(origDir, dirName);
    newDir->cd();
    // Extract the baseKeys, create histograms for each
    for (auto it = meta.begin(); it != meta.end(); ++it) {
        std::string baseKey = it.key();
        baseKeys_.push_back(baseKey);

        // Create histograms
        createHistogramsFor(baseKey);
    }

    std::cout << "[HistGivenBoth] Initialized " << baseKeys_.size() << " baseKeys from " << metadataJsonPath_ << std::endl;
    std::cout << "Initialized HistGivenBoth histograms in directory: " << dirName << std::endl;
    origDir->cd();
}

void HistGivenBoth::createHistogramsFor(const std::string& baseKey) {
    // We'll store them in histMap_[baseKey].
    // The names will be something like: hCorrV1_{baseKey}, hCorrV2_{baseKey}, pDiff_{baseKey}
    // Make sure to sanitize baseKey for histogram names if it has special characters.

    HistGivenBothSet hset;
    std::string safeKey = baseKey;
    // Replace special characters to avoid ROOT conflicts
    for (auto &c: safeKey) {
        if (c == ':') c = '_';
        if (c == '/') c = '_';
        if (c == ' ') c = '_';
    }
    
    double nPtBins_ = 100;
    double ptMin_  = 0.0;
    double ptMax_  = 1000.0;

    int binN = 100;
    double binMin = -0.5;
    double binMax =  0.5;

    if (baseKey.find("_L1FastJet_") != std::string::npos ||
        baseKey.find("_L2Relative_") != std::string::npos|| 
        baseKey.find("_L3Absolute_") != std::string::npos|| 
        baseKey.find("_L2L3Residual_") != std::string::npos){
        binMin = 0.5;
        binMax = 1.5;
    }
    hset.hCorrV1 = new TH1D(
        ("hCorrV1_" + safeKey).c_str(),
        (baseKey + " : V1 Correction Factor").c_str(),
        binN, binMin, binMax
    );
    hset.hCorrV1->GetXaxis()->SetTitle("Correction Factor (V1)");
    hset.hCorrV1->GetYaxis()->SetTitle("Events");

    hset.hCorrV2 = new TH1D(
        ("hCorrV2_" + safeKey).c_str(),
        (baseKey + " : V2 Correction Factor").c_str(),
        binN, binMin, binMax
    );
    hset.hCorrV2->GetXaxis()->SetTitle("Correction Factor (V2)");
    hset.hCorrV2->GetYaxis()->SetTitle("Events");

    // Store in map
    histMap_[baseKey] = hset;
}

void HistGivenBoth::fill(const std::string& baseKey, const std::vector<double>& corrFactors) {
    // Expect corrFactors.size() >= 2 (V1, V2). 
    // If the user wants more versions, they'd handle it similarly.

    if (histMap_.find(baseKey) == histMap_.end()) {
        // Not found; might indicate the baseKey wasn't in metadata.
        return;
    }

    auto &hset = histMap_[baseKey];
    
    if (corrFactors.size() < 2) {
        // If we only have 1 or zero corrections, we can't fill V1/V2 properly
        // but let's guard anyway
        return;
    }

    double corrV1 = corrFactors[0];
    double corrV2 = corrFactors[1];

    hset.hCorrV1->Fill(corrV1);
    hset.hCorrV2->Fill(corrV2);

}

