#ifndef SKIMTREE_H
#define SKIMTREE_H

#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "GlobalFlag.h"

class SkimTree{
public:
    // Constructor accepting a reference to GlobalFlag
    explicit SkimTree(GlobalFlag& globalFlags);
    ~SkimTree();

    // Tree operations
    Long64_t getEntries() const;
    TChain* getChain() const;  // Getter function to access fChain_
    Int_t getEntry(Long64_t entry);
    Long64_t loadEntry(Long64_t entry);

    // Input handling
    void setInput(const std::string& outName);
    void loadInput();

    void setInputJsonPath(const std::string& inDir);
    void loadInputJson();

    void loadJobFileNames();
    void loadTree();

    // Accessors for tree variables (public for direct access)
    // {} in the end is to initialise
    // Event information
    UInt_t run{};
    UInt_t luminosityBlock{};
    ULong64_t event{};
    UInt_t bunchCrossing{}; // NanoV12

    // MET information
    Float_t ChsMET_phi{};      // Run2
    Float_t ChsMET_pt{};       // Run2

    // Jet variables
    static const int nJetMax = 200;
    Int_t nJet{}; // NanoV12
    Float_t Jet_pt[nJetMax]{};
    Float_t Jet_eta[nJetMax]{};
    Float_t Jet_phi[nJetMax]{};
    Float_t Jet_mass[nJetMax]{};

    Float_t Jet_rawFactor[nJetMax]{};
    Float_t Jet_area[nJetMax]{};
    UChar_t Jet_jetId[nJetMax]{}; // NanoV12

    // Other variables
    Float_t Rho{}; // Run2, Run3


private:
    // Member variables
    std::string outName_;
    std::string loadedSampKey_ = "MC_Year_Channel_Name";
    int loadedNthJob_ = 1;
    int loadedTotJob_ = 100;
    std::string inputJsonPath_ = "./FilesSkim_2022_GamJet.json";
    std::vector<std::string> loadedAllFileNames_;
    std::vector<std::string> loadedJobFileNames_;

    Int_t fCurrent_; // Current Tree number in a TChain

    // ROOT TChain
    std::unique_ptr<TChain> fChain_;

    // Disable copying and assignment
    SkimTree(const SkimTree&) = delete;
    SkimTree& operator=(const SkimTree&) = delete;

    // Reference to GlobalFlag instance
    GlobalFlag& globalFlags_;
    const GlobalFlag::Year year_;
    const GlobalFlag::Era era_;
    const GlobalFlag::Channel channel_;
    const bool isDebug_;
    const bool isData_;
    const bool isMC_;
};

#endif // SKIMTREE_H

