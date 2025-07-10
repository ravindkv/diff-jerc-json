#ifndef HISTGIVENETA_H
#define HISTGIVENETA_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include "TH1D.h"
#include "TProfile.h"
#include "TFile.h"
#include "Helper.h"
#include "nlohmann/json.hpp"

struct HistGivenEtaSet {
    TH1D* hCorrOld = nullptr;
    TH1D* hCorrNew = nullptr;
    TProfile* pCorrOld = nullptr;
    TProfile* pCorrNew = nullptr;
};

class HistGivenEta {
public:
    // Constructor: pass metadata JSON path, output file name, etc.
    HistGivenEta(TDirectory *origDir, const std::string& directoryName, const std::string& metadataJsonPath);
    ~HistGivenEta();

    // Initialize histograms by reading the metadata JSON
    void initialize(TDirectory *origDir, const std::string& directoryName, const std::string& metadataJsonPath);

    // Fill the histograms for a given baseKey, given the vector of corrections
    // (corrFactors[0] = V1, corrFactors[1] = V2) plus the jet pT
    void fill(const std::string& baseKey, double jetPt, const std::vector<double>& corrFactors);

    // Write out histograms to the TFile
    void save();

private:
    std::string metadataJsonPath_;
    // We parse the metadata and create a map from baseKey -> histograms
    std::unordered_map<std::string, HistGivenEtaSet> histMap_;

    // Cache of baseKeys from metadata
    std::vector<std::string> baseKeys_;

    // Internal helper to create the needed TH1D / TProfile
    // for each baseKey. Called during initialize().
    void createHistogramsFor(const std::string& baseKey);
};

#endif // HISTGIVENETA_H

