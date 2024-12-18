#include "EventPick.h"

// Constructor implementation
EventPick::EventPick(GlobalFlag& globalFlags) : 
    globalFlags_(globalFlags),
    year_(globalFlags_.getYear()),
    channel_(globalFlags_.getChannel()),
    isMC_(globalFlags_.isMC()),
    isDebug_(globalFlags_.isDebug()){
}

// Destructor
EventPick::~EventPick() {
    std::cout << "Destructor of EventPick" << '\n';
}

// Helper function for printing debug messages
void EventPick::printDebug(const std::string& message) const {
    if (isDebug_) {
        std::cout << message << '\n';
    }
}

// Function to check High-Level Triggers (HLT)
auto EventPick::passHLT(const std::shared_ptr<SkimTree>& tree) const -> bool {
    bool pass_HLT = false;

    if (channel_ == GlobalFlag::Channel::ZeeJet) {
        if (year_ == GlobalFlag::Year::Year2016Pre || year_ == GlobalFlag::Year::Year2016Post) {
            pass_HLT = tree->HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ;
        } else if (year_ == GlobalFlag::Year::Year2017 || year_ == GlobalFlag::Year::Year2018) {
            pass_HLT = tree->HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL;
        }
    }//ZeeJet

    printDebug("pass_HLT = " + std::to_string(pass_HLT));
    return pass_HLT;
}


// Function to apply event filters
auto EventPick::passFilter(const std::shared_ptr<SkimTree>& tree) const -> bool {
    bool pass = tree->Flag_goodVertices &&
                tree->Flag_globalSuperTightHalo2016Filter &&
                tree->Flag_HBHENoiseFilter &&
                tree->Flag_HBHENoiseIsoFilter &&
                tree->Flag_EcalDeadCellTriggerPrimitiveFilter &&
                tree->Flag_BadPFMuonFilter &&
                tree->Flag_eeBadScFilter;

    if (year_ == GlobalFlag::Year::Year2017 || year_ == GlobalFlag::Year::Year2018) {
        pass = pass && tree->Flag_ecalBadCalibFilter;
    }

    // Debugging output
    if (isDebug_) {
        printDebug("Event Filters:");
        printDebug("Flag_goodVertices: " + std::to_string(tree->Flag_goodVertices));
        printDebug("Flag_globalSuperTightHalo2016Filter: " + std::to_string(tree->Flag_globalSuperTightHalo2016Filter));
        printDebug("Flag_HBHENoiseFilter: " + std::to_string(tree->Flag_HBHENoiseFilter));
        printDebug("Flag_HBHENoiseIsoFilter: " + std::to_string(tree->Flag_HBHENoiseIsoFilter));
        printDebug("Flag_EcalDeadCellTriggerPrimitiveFilter: " + std::to_string(tree->Flag_EcalDeadCellTriggerPrimitiveFilter));
        printDebug("Flag_BadPFMuonFilter: " + std::to_string(tree->Flag_BadPFMuonFilter));
        printDebug("Flag_eeBadScFilter: " + std::to_string(tree->Flag_eeBadScFilter));
        if (year_ == GlobalFlag::Year::Year2017 || year_ == GlobalFlag::Year::Year2018) {
            printDebug("Flag_ecalBadCalibFilter: " + std::to_string(tree->Flag_ecalBadCalibFilter));
        }
    }

    return pass;
}


