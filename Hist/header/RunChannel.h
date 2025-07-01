
#pragma once

#include <iostream>
#include <cmath>

// ROOT includes
#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TLorentzVector.h>

// User-defined includes
#include "SkimTree.h"
#include "GlobalFlag.h"

class RunChannel{
public:
    // Constructor accepting a reference to GlobalFlag
    explicit RunChannel(GlobalFlag& globalFlags);
    ~RunChannel() = default;

    int Run(std::shared_ptr<SkimTree>& skimT, const std::string& metadataJsonPath, TFile* fout);

private:
    // Reference to GlobalFlag instance
    GlobalFlag& globalFlags_;

    // Add any private member variables or methods here if needed
};


