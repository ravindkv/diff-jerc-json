#ifndef HISTGIVENBOTH_H
#define HISTGIVENBOTH_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include "TH1D.h"
#include "TProfile.h"
#include "TFile.h"
#include "Helper.h"
#include "nlohmann/json.hpp"

/**
 * HistGivenBothSet is a helper struct to group the three histograms
 * we want per baseKey:
 *   - TH1D for V1 corrections
 *   - TH1D for V2 corrections
 *   - TProfile for the difference (V1 - V2)
 */
struct HistGivenBothSet {
    TH1D* hCorrOld = nullptr;
    TH1D* hCorrNew = nullptr;
};

class HistGivenBoth {
public:
    // Constructor: pass metadata JSON path, output file name, etc.
    HistGivenBoth(TDirectory *origDir, const std::string& directoryName, const std::string& metadataJsonPath);
    ~HistGivenBoth();

    // Initialize histograms by reading the metadata JSON
    void initialize(TDirectory *origDir, const std::string& directoryName, const std::string& metadataJsonPath);

    // Fill the histograms for a given baseKey, given the vector of corrections
    // (corrFactors[0] = V1, corrFactors[1] = V2) plus the jet pT
    void fill(const std::string& baseKey, const std::vector<double>& corrFactors);

    // Write out histograms to the TFile
    void save();

private:
    std::string metadataJsonPath_;
    // We parse the metadata and create a map from baseKey -> histograms
    std::unordered_map<std::string, HistGivenBothSet> histMap_;

    // Cache of baseKeys from metadata
    std::vector<std::string> baseKeys_;

    // Internal helper to create the needed TH1D / TProfile
    // for each baseKey. Called during initialize().
    void createHistogramsFor(const std::string& baseKey);
};

#endif // HISTGIVENBOTH_H

