#include "FillHist.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

#include "TDirectory.h"
#include "TROOT.h"

FillHist::FillHist(const std::string& metadataJsonPath,
                   int nPtBins, double ptMin, double ptMax)
    : metadataJsonPath_(metadataJsonPath),
      nPtBins_(nPtBins),
      ptMin_(ptMin),
      ptMax_(ptMax)
{
    // We delay the opening of the TFile until initialize()
}

FillHist::~FillHist() {
}

void FillHist::initialize() {
    // Read metadata JSON
    std::ifstream inFile(metadataJsonPath_);
    if (!inFile.is_open()) {
        throw std::runtime_error("FillHist::initialize: Unable to open metadata JSON: " + metadataJsonPath_);
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

    // Extract the baseKeys, create histograms for each
    for (auto it = meta.begin(); it != meta.end(); ++it) {
        std::string baseKey = it.key();
        baseKeys_.push_back(baseKey);

        // Create histograms
        createHistogramsFor(baseKey);
    }

    std::cout << "[FillHist] Initialized " << baseKeys_.size() << " baseKeys from " << metadataJsonPath_ << std::endl;
}

void FillHist::createHistogramsFor(const std::string& baseKey) {
    // We'll store them in histMap_[baseKey].
    // The names will be something like: hCorrV1_{baseKey}, hCorrV2_{baseKey}, pDiff_{baseKey}
    // Make sure to sanitize baseKey for histogram names if it has special characters.

    HistogramSet hset;
    std::string safeKey = baseKey;
    // Replace special characters to avoid ROOT conflicts
    for (auto &c: safeKey) {
        if (c == ':') c = '_';
        if (c == '/') c = '_';
        if (c == ' ') c = '_';
    }
    
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

    // TProfile for the difference: (V1 - V2) vs Jet pT
    hset.pDiff = new TProfile(
        ("pDiff_" + safeKey).c_str(),
        (baseKey + " : (V1 - V2) vs pT").c_str(),
        nPtBins_, ptMin_, ptMax_
    );
    hset.pDiff->GetXaxis()->SetTitle("Jet p_{T} [GeV]");
    hset.pDiff->GetYaxis()->SetTitle("V1 - V2");

    // Store in map
    histMap_[baseKey] = hset;
}

void FillHist::fill(const std::string& baseKey, double jetPt, const std::vector<double>& corrFactors) {
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
    double diff   = 100*(corrV1 - corrV2)/corrV1;

    // Fill the TH1Ds vs Jet pT, but the Y value is the correction factor
    // We're essentially making a profile, but in TH1D form: 
    //   x-axis = pT, weight = correction factor
    // Typically, you'd just store pT of "corrected jets," but let's follow the request literally.
    hset.hCorrV1->Fill(corrV1);
    hset.hCorrV2->Fill(corrV2);

    // Fill TProfile with (diff) vs Jet pT
    hset.pDiff->Fill(jetPt, diff);
}

