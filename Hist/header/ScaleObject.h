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

    // === New metadata approach ===
    void loadMetadata(const std::string& metadataJsonPath);
    std::vector<double> evaluateCorrections(const std::string& baseKey,
                                            const std::vector<double>& inputs) const;

    std::vector<double> evaluateJerSF(const std::string& baseKey, const double& jetEta, const double& jetPt, const std::string &syst) const;
    // Alternatively, you can create more specialized evaluate methods, e.g. evaluateJecPt()

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

    // Keep track of loaded CorrectionSets (to avoid re-opening the same JSON multiple times)
    std::unordered_map<std::string, std::shared_ptr<correction::CorrectionSet>> correctionSets_;

    // Helper method to load a Correction::Ref from a JSON+tag, caching the CorrectionSet
    correction::Correction::Ref getCorrectionRef(const std::string& jsonFile, const std::string& tag);

};

#endif

