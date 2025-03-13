#ifndef SCALEOBJECT_H
#define SCALEOBJECT_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "SkimTree.h"
#include "correction.h"         // Provided by correctionlib
#include "GlobalFlag.h"

#include "nlohmann/json.hpp"

// Struct to hold info about one correction entry (V1, V2, etc.)
struct CorrectionInfo {
    std::string jsonFilename;
    std::string correctionTag;
    correction::Correction::Ref corrRef; // correctionlib reference
};

class ScaleObject {
public: 
    explicit ScaleObject(GlobalFlag& globalFlags);
    ~ScaleObject() {}

    // Evaluate a single correction given the json path, correction tag, and input parameters
    double evaluateCorrection(const std::string& jsonFile,
                          const std::string& correctionTag,
                          const std::vector<double>& inputs) const;

    double evaluateJerSF(const std::string& jsonFile,
                                  const std::string& correctionTag,
                                  const double& jetEta,
                                  const double& jetPt,
                                  const std::string &syst) const;


private:
    GlobalFlag& globalFlags_;
    const GlobalFlag::Year year_;
    const GlobalFlag::Era era_;
    const GlobalFlag::Channel channel_;
    const bool isDebug_;
    const bool isData_;
    const bool isMC_;

    // === Data members for new metadata approach ===

    // For each "baseKey" (e.g. "DATA_L1FastJet_AK4PFPuppi"),
    // we can have multiple CorrectionInfo entries (e.g. for V1, V2).
    std::unordered_map<std::string, std::vector<CorrectionInfo>> metadataMap_;

    // In your private members:
    mutable std::unordered_map<std::string, std::shared_ptr<correction::CorrectionSet>> correctionSets_;

    // Modify the declaration of getCorrectionRef:
    correction::Correction::Ref getCorrectionRef(const std::string& jsonFile, const std::string& tag) const;

};

#endif

