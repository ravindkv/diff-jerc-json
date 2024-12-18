#ifndef HISTZEEJET_H
#define HISTZEEJET_H

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
#include "EventPick.h"
#include "ObjectPick.h"
#include "ScaleObject.h"
#include "GlobalFlag.h"

class RunZeeJet{
public:
    // Constructor accepting a reference to GlobalFlag
    explicit RunZeeJet(GlobalFlag& globalFlags);
    ~RunZeeJet() = default;

    int Run(std::shared_ptr<SkimTree>& skimT, EventPick* eventP, ObjectPick* objP, ScaleObject* scaleObject, const std::string& metadataJsonPath, TFile* fout);

private:
    // Reference to GlobalFlag instance
    GlobalFlag& globalFlags_;

    // Add any private member variables or methods here if needed
};

#endif // HISTZEEJET_H

