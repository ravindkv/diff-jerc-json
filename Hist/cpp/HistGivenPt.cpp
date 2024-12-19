#include "HistGivenPt.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

#include "TDirectory.h"
#include "TROOT.h"

HistGivenPt::HistGivenPt(TDirectory *origDir, const std::string& directoryName,const std::string& metadataJsonPath)
{
    initialize(origDir, directoryName, metadataJsonPath);
}

HistGivenPt::~HistGivenPt() {
}

void HistGivenPt::initialize(TDirectory *origDir, const std::string& directoryName, const std::string& metadataJsonPath){
    // Read metadata JSON
    std::ifstream inFile(metadataJsonPath);
    if (!inFile.is_open()) {
        throw std::runtime_error("HistGivenPt::initialize: Unable to open metadata JSON: " + metadataJsonPath_);
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
    std::string dirName = "HistGivenPt/"+ directoryName;
    TDirectory* newDir = Helper::createTDirectory(origDir, dirName);
    newDir->cd();
    // Extract the baseKeys, create histograms for each
    for (auto it = meta.begin(); it != meta.end(); ++it) {
        std::string baseKey = it.key();
        baseKeys_.push_back(baseKey);

        // Create histograms
        createHistogramsFor(baseKey);
    }

    std::cout << "[HistGivenPt] Initialized " << baseKeys_.size() << " baseKeys from " << metadataJsonPath_ << std::endl;
    std::cout << "Initialized HistGivenPt histograms in directory: " << dirName << std::endl;
    origDir->cd();
}

void HistGivenPt::createHistogramsFor(const std::string& baseKey) {
    // We'll store them in histMap_[baseKey].
    // The names will be something like: hCorrV1_{baseKey}, hCorrV2_{baseKey}, pDiff_{baseKey}
    // Make sure to sanitize baseKey for histogram names if it has special characters.

    HistGivenPtSet hset;
    std::string safeKey = baseKey;
    // Replace special characters to avoid ROOT conflicts
    for (auto &c: safeKey) {
        if (c == ':') c = '_';
        if (c == '/') c = '_';
        if (c == ' ') c = '_';
    }
    
     std::vector<double> binsEta = {
        -5.191, -3.839, -3.489, -3.139, -2.964, -2.853,
        -2.650, -2.500, -2.322, -2.172, -1.930, -1.653,
        -1.479, -1.305, -1.044, -0.783, -0.522, -0.261,
        0.000, 0.261, 0.522, 0.783, 1.044, 1.305, 1.479,
        1.653, 1.930, 2.172, 2.322, 2.500, 2.650, 2.853,
        2.964, 3.139, 3.489, 3.839, 5.191
    };
    const int nEta = binsEta.size() - 1;

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

    // TProfile 
    hset.pCorrV1 = new TProfile(
        ("pCorrV1_" + safeKey).c_str(),
        (baseKey + " : CorrV1 vs #eta").c_str(),
        nEta, binsEta.data() 
    );
    hset.pCorrV1->GetXaxis()->SetTitle("Jet #eta");
    hset.pCorrV1->GetYaxis()->SetTitle("Mean of CorrV1");

    hset.pCorrV2 = new TProfile(
        ("pCorrV2_" + safeKey).c_str(),
        (baseKey + " : CorrV2 vs #eta").c_str(),
        nEta, binsEta.data() 
    );
    hset.pCorrV2->GetXaxis()->SetTitle("Jet #eta");
    hset.pCorrV2->GetYaxis()->SetTitle("Mean of CorrV2");

    // Store in map
    histMap_[baseKey] = hset;
}

void HistGivenPt::fill(const std::string& baseKey, double jetEta, const std::vector<double>& corrFactors) {
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

    // Fill TH1 
    hset.hCorrV1->Fill(corrV1);
    hset.hCorrV2->Fill(corrV2);

    // Fill TProfile 
    hset.pCorrV1->Fill(jetEta, corrV1);
    hset.pCorrV2->Fill(jetEta, corrV2);
}

