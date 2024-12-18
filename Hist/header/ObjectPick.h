#ifndef OBJECTPICK_H
#define OBJECTPICK_H

#include <vector>
#include <string>
#include <set>
#include <iostream>
#include <fstream>
#include <memory>
#include <cmath>

#include <TH1F.h>
#include <TH1D.h>
#include <TLorentzVector.h>

#include "SkimTree.h"
#include "GlobalFlag.h"

class ObjectPick{
public:
    // Constructor accepting a reference to GlobalFlag
    explicit ObjectPick(GlobalFlag& globalFlags);
    ~ObjectPick();

    // Clear picked objects
    void clearObjects();

    // Reco objects
    void pickElectrons(const SkimTree& skimT);
    void pickMuons(const SkimTree& skimT);
    void pickPhotons(const SkimTree& skimT);
    void pickRefs(const SkimTree& skimT);
    void pickJets(const SkimTree& skimT, const TLorentzVector& p4Ref);

    // Gen objects
    void pickGenElectrons(const SkimTree& skimT);
    void pickGenMuons(const SkimTree& skimT);
    void pickGenPhotons(const SkimTree& skimT);
    void pickGenRefs(const SkimTree& skimT, const TLorentzVector& p4Ref);
    void pickGenJets(const SkimTree& skimT);

    // Accessors for picked objects
    const std::vector<int>& getPickedElectrons() const;
    const std::vector<int>& getPickedMuons() const;
    const std::vector<int>& getPickedPhotons() const;
    const std::vector<TLorentzVector>& getPickedRefs() const;
    const std::vector<TLorentzVector>& getPickedJetsP4() const;
    const std::vector<int>& getPickedJetsIndex() const;

    const std::vector<int>& getPickedGenElectrons() const;
    const std::vector<int>& getPickedGenMuons() const;
    const std::vector<int>& getPickedGenPhotons() const;
    const std::vector<TLorentzVector>& getPickedGenRefs() const;
    const std::vector<TLorentzVector>& getPickedGenJets() const;

private:

    // Reco objects
    std::vector<int> pickedElectrons_;
    std::vector<int> pickedMuons_;
    std::vector<int> pickedPhotons_;
    std::vector<TLorentzVector> pickedRefs_;
    std::vector<TLorentzVector> pickedJetsP4_;
    std::vector<int> pickedJetsIndex_;

    // Gen objects
    std::vector<int> pickedGenElectrons_;
    std::vector<int> pickedGenMuons_;
    std::vector<int> pickedGenPhotons_;
    std::vector<TLorentzVector> pickedGenRefs_;
    std::vector<TLorentzVector> pickedGenJets_;

    // Reference to GlobalFlag instance
    GlobalFlag& globalFlags_;
    const GlobalFlag::Year year_;
    const GlobalFlag::Channel channel_;
    const bool isDebug_;

    // Helper function for debug printing
    void printDebug(const std::string& message) const;
};

#endif  // OBJECTPICK_H

