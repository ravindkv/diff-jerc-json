#include "ScaleObject.h"
#include "Helper.h" 
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

// Modify the definition to be const:
correction::Correction::Ref ScaleObject::getCorrectionRef(const std::string& jsonFile, const std::string& tag) const {
    if (correctionSets_.find(jsonFile) == correctionSets_.end()) {
        std::cout << "Loading CorrectionSet from: " << jsonFile << std::endl;
        auto cset = correction::CorrectionSet::from_file(jsonFile);
        correctionSets_[jsonFile] = std::move(cset); // Allowed since correctionSets_ is mutable.
    }
    try {
        return correctionSets_.at(jsonFile)->at(tag);
    } catch (const std::exception &e) {
        std::cerr << "Error: could not find tag '" << tag << "' in JSON file '" << jsonFile << "'\n";
        throw;
    }
}

// Updated evaluateCorrection with debug prints
double ScaleObject::evaluateCorrection(const std::string& jsonFile,
                                         const std::string& correctionTag,
                                         const std::vector<double>& inputs) const {
    // Print inputs if debugging
    if (isDebug_) {
        std::cout<< "[DEBUG] inputs=["; 
        for (size_t i = 0; i < inputs.size(); ++i) {
            std::cout << inputs[i];
            if (i + 1 < inputs.size()) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }

    // Define the variant type expected by correctionlib
    using CorrType = std::variant<int, double, std::string>;
    std::vector<CorrType> formattedInputs;
    formattedInputs.reserve(inputs.size());
    for (const double& value : inputs) {
        formattedInputs.emplace_back(value);
    }

    // Get the correction reference (caching happens in getCorrectionRef)
    correction::Correction::Ref corrRef = getCorrectionRef(jsonFile, correctionTag);

    try {
        // Evaluate the correction factor
        double result = corrRef->evaluate(formattedInputs);
        if (isDebug_) {
            std::cout << "result: " << result << std::endl;
        }
        return result;
    } catch (const std::exception &e) {
        std::cerr << "Error: evaluateCorrection for json=" << jsonFile
                  << " tag=" << correctionTag << ": " << e.what() << std::endl;
        return 1.0;
    }
}

// Updated evaluateJerSF with debug prints
double ScaleObject::evaluateJerSF(const std::string& jsonFile,
                                  const std::string& correctionTag,
                                  const double& jetEta,
                                  const double& jetPt,
                                  const std::string &syst) const {
    if (isDebug_) {
        std::cout << "[DEBUG] inputs: jetEta=" << jetEta
                  << ", jetPt=" << jetPt
                  << ", syst='" << syst << "'" << std::endl;
    }

    using CorrType = std::variant<int, double, std::string>;
    std::vector<CorrType> formattedInputs;
    // Fill the inputs: note that syst is a string, while jetEta and jetPt are doubles.
    formattedInputs.emplace_back(jetEta);
    formattedInputs.emplace_back(jetPt);
    formattedInputs.emplace_back(syst);

    // Get the correction reference using the caching mechanism.
    correction::Correction::Ref corrRef = getCorrectionRef(jsonFile, correctionTag);

    try {
        // Evaluate and return the scale factor.
        double result = corrRef->evaluate(formattedInputs);
        if (isDebug_) {
            std::cout << "[DEBUG] result: " << result << std::endl;
        }
        return result;
    } catch (const std::exception &e) {
        std::cerr << "Error: evaluateJerSF for json=" << jsonFile
                  << " tag=" << correctionTag << ": " << e.what() << std::endl;
        return 1.0;
    }
}

