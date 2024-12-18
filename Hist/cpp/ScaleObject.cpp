#include "ScaleObject.h"
#include "Helper.h" // Not shown, presumably your own utilities
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <variant> // Needed for std::variant
#include "nlohmann/json.hpp"

ScaleObject::ScaleObject(GlobalFlag& globalFlags)
    : globalFlags_(globalFlags)
    , year_(globalFlags_.getYear())
    , era_(globalFlags_.getEra())
    , channel_(globalFlags_.getChannel())
    , isDebug_(globalFlags_.isDebug())
    , isData_(globalFlags_.isData())
    , isMC_(globalFlags_.isMC())
{
    // Constructor body (empty or do any initialization needed)
}

//===-----------------------------------------------------------------===//
// New approach: parse metadata.json to load multiple corrections (V1, V2)
//===-----------------------------------------------------------------===//

void ScaleObject::loadMetadata(const std::string& metadataJsonPath) {
    std::cout << "==> loadMetadata from: " << metadataJsonPath << std::endl;

    // Parse the metadata JSON
    std::ifstream inFile(metadataJsonPath);
    if (!inFile.is_open()) {
        throw std::runtime_error("Error: Unable to open metadata JSON: " + metadataJsonPath);
    }
    
    nlohmann::json meta;
    inFile >> meta;
    inFile.close();

    // meta is of structure:
    // {
    //   "DATA_L1FastJet_AK4PFPuppi": [
    //       ["Winter24Prompt24_V1.json", "Winter24Prompt24_RunBCD_V1_DATA_L1FastJet_AK4PFPuppi"],
    //       ["Winter24Prompt24_V2.json", "Winter24Prompt24_RunBCD_V2_DATA_L1FastJet_AK4PFPuppi"]
    //   ],
    //   ...
    // }

    for (auto it = meta.begin(); it != meta.end(); ++it) {
        std::string baseKey = it.key();  // e.g. "DATA_L1FastJet_AK4PFPuppi"
        auto entries = it.value();       // array of [ [jsonFile, tag], [jsonFile, tag], ... ]
        
        std::vector<CorrectionInfo> corrInfos;

        for (auto& entry : entries) {
            // entry is e.g. ["Winter24Prompt24_V1.json", "Winter24Prompt24_RunBCD_V1_DATA_L1FastJet_AK4PFPuppi"]
            if (entry.size() < 2) {
                // Possibly invalid structure
                corrInfos.push_back( CorrectionInfo{"", "", nullptr} );
                continue;
            }
            std::string jsonFilename  = entry.at(0).get<std::string>();
            std::string correctionTag = entry.at(1).is_null() ? "" : entry.at(1).get<std::string>();
            if (correctionTag.empty()) {
                // Means the tag is absent in this version
                corrInfos.push_back( CorrectionInfo{jsonFilename, "", nullptr} );
                continue;
            }

            // Now load the CorrectionRef
            correction::Correction::Ref corrRef = getCorrectionRef(jsonFilename, correctionTag);

            CorrectionInfo ci{jsonFilename, correctionTag, corrRef};
            corrInfos.push_back(ci);
        }

        // Store in the metadataMap_
        metadataMap_[baseKey] = corrInfos;

        if (isDebug_) {
            std::cout << "BaseKey: " << baseKey << ", # of versions: " << corrInfos.size() << "\n";
            for (size_t i = 0; i < corrInfos.size(); ++i) {
                std::cout << "  [" << i << "] JSON: " << corrInfos[i].jsonFilename
                          << ", Tag: " << corrInfos[i].correctionTag << "\n";
            }
        }
    }
}

// Helper function to load a CorrectionSet only once
correction::Correction::Ref ScaleObject::getCorrectionRef(const std::string& jsonFile, const std::string& tag) {
    // If the CorrectionSet for this JSON is not yet cached, load it
    if (correctionSets_.find(jsonFile) == correctionSets_.end()) {
        std::cout << "Loading CorrectionSet from: " << jsonFile << std::endl;
        // If the JSON might be gzipped, ensure correctionlib can handle it
        auto cset = correction::CorrectionSet::from_file(jsonFile);
        correctionSets_[jsonFile] = std::move(cset);
    }
    // Return the reference
    try {
        return correctionSets_[jsonFile]->at(tag);
    } catch(const std::exception &e) {
        std::cerr << "Error: could not find tag '" << tag << "' in JSON file '" << jsonFile << "'\n";
        throw;
    }
}

// Evaluate all corrections for a given baseKey
// e.g. evaluate [V1, V2, ...] with the same input parameters
// Evaluate corrections for a given baseKey, converting inputs to correctionlib format
std::vector<double> ScaleObject::evaluateCorrections(const std::string& baseKey,
                                                     const std::vector<double>& inputs) const
{
    // Define the variant type expected by correctionlib
    using CorrType = std::variant<int, double, std::string>;

    // Convert inputs from std::vector<double> to std::vector<CorrType>
    std::vector<CorrType> formattedInputs;
    formattedInputs.reserve(inputs.size());

    for (const double& value : inputs) {
        formattedInputs.emplace_back(value);
    }

    // Results to store correction factors
    std::vector<double> results;

    // Find the baseKey in the metadata map
    auto it = metadataMap_.find(baseKey);
    if (it == metadataMap_.end()) {
        // No entry for this baseKey
        return results;
    }

    const auto& corrInfos = it->second;
    for (const auto& info : corrInfos) {
        if (!info.corrRef) {
            // Tag was missing or invalid; default to 1.0
            if(isDebug_)std::cerr << "Error: evaluateCorrections for baseKey=" << baseKey
                          << " with tag=" << info.correctionTag << ": " << std::endl;
            results.push_back(1.0);
        } else {
            try {
                double factor = info.corrRef->evaluate(formattedInputs);
                results.push_back(factor);
            } catch (const std::exception &e) {
                std::cerr << "Error: evaluateCorrections for baseKey=" << baseKey
                          << " with tag=" << info.correctionTag << ": " << e.what() << std::endl;
                results.push_back(1.0);
            }
        }
    }

    return results;
}

std::vector<double> ScaleObject::evaluateJerSF(const std::string& baseKey, const double& jetEta, const double& jetPt, const std::string &syst) const
{
    // Results to store correction factors
    std::vector<double> results;

    // Find the baseKey in the metadata map
    auto it = metadataMap_.find(baseKey);
    if (it == metadataMap_.end()) {
        // No entry for this baseKey
        return results;
    }

    const auto& corrInfos = it->second;
    for (const auto& info : corrInfos) {
        if (!info.corrRef) {
            // Tag was missing or invalid; default to 1.0
            if(isDebug_) std::cerr << "Error: evaluateCorrections for baseKey=" << baseKey
                          << " with tag=" << info.correctionTag << ": " << std::endl;
            results.push_back(1.0);
        } else {
            try {
                double factor = info.corrRef->evaluate({jetEta, jetPt, syst});
                results.push_back(factor);
            } catch (const std::exception &e) {
                std::cerr << "Error: evaluateCorrections for baseKey=" << baseKey
                          << " with tag=" << info.correctionTag << ": " << e.what() << std::endl;
                results.push_back(1.0);
            }
        }
    }

    return results;
}
