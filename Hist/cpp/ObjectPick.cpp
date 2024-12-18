#include "ObjectPick.h"

// Constructor implementation
ObjectPick::ObjectPick(GlobalFlag& globalFlags) : 
    globalFlags_(globalFlags),
    year_(globalFlags_.getYear()),
    channel_(globalFlags_.getChannel()),
    isDebug_(globalFlags_.isDebug())
{
}
// Destructor
ObjectPick::~ObjectPick() {
    // Cleanup if necessary
}

// Clear picked objects
void ObjectPick::clearObjects() {
    pickedElectrons_.clear();
    pickedMuons_.clear();
    pickedPhotons_.clear();
    pickedRefs_.clear();
    pickedJetsP4_.clear();
    pickedJetsIndex_.clear();

    pickedGenElectrons_.clear();
    pickedGenMuons_.clear();
    pickedGenPhotons_.clear();
    pickedGenRefs_.clear();
    pickedGenJets_.clear();
}

// Helper function for debug printing
void ObjectPick::printDebug(const std::string& message) const {
    if (isDebug_) {
        std::cout << message << '\n';
    }
}

// Accessors for picked objects
auto ObjectPick::getPickedElectrons() const -> const std::vector<int>& {
    return pickedElectrons_;
}

auto ObjectPick::getPickedMuons() const -> const std::vector<int>& {
    return pickedMuons_;
}

auto ObjectPick::getPickedPhotons() const -> const std::vector<int>& {
    return pickedPhotons_;
}

auto ObjectPick::getPickedRefs() const -> const std::vector<TLorentzVector>& {
    return pickedRefs_;
}

auto ObjectPick::getPickedJetsP4() const -> const std::vector<TLorentzVector>& {
    return pickedJetsP4_;
}

auto ObjectPick::getPickedJetsIndex() const -> const std::vector<int>& {
    return pickedJetsIndex_;
}

auto ObjectPick::getPickedGenElectrons() const -> const std::vector<int>& {
    return pickedGenElectrons_;
}

auto ObjectPick::getPickedGenMuons() const -> const std::vector<int>& {
    return pickedGenMuons_;
}

auto ObjectPick::getPickedGenPhotons() const -> const std::vector<int>& {
    return pickedGenPhotons_;
}

auto ObjectPick::getPickedGenRefs() const -> const std::vector<TLorentzVector>& {
    return pickedGenRefs_;
}

auto ObjectPick::getPickedGenJets() const -> const std::vector<TLorentzVector>& {
    return pickedGenJets_;
}

// Reco objects
void ObjectPick::pickMuons(const SkimTree& skimT) {
    printDebug("Starting Selection, nMuon = "+std::to_string(skimT.nMuon));

    for (UInt_t m = 0; m < skimT.nMuon; ++m) {
        double eta = skimT.Muon_eta[m];
        double pt = skimT.Muon_pt[m];

        bool passPrompt = false;
        if (pt > 20) {
            passPrompt = (std::abs(eta) <= 2.3 &&
                          skimT.Muon_tightId[m] &&
                          skimT.Muon_pfRelIso04_all[m] < 0.15 &&
                          skimT.Muon_dxy[m] < 0.2 &&
                          skimT.Muon_dz[m] < 0.5);
        }

        if (passPrompt) {
            pickedMuons_.push_back(m);
            printDebug("Muon " + std::to_string(m) + " selected: pt = " + std::to_string(pt) + ", eta = " + std::to_string(eta));
        } else {
            printDebug("Muon " + std::to_string(m) + " rejected: pt = " + std::to_string(pt) + ", eta = " + std::to_string(eta));
        }
    }

    printDebug("Total Muons Selected: " + std::to_string(pickedMuons_.size()));
}

void ObjectPick::pickElectrons(const SkimTree& skimT) {
    printDebug("Starting Selection, nElectron = "+std::to_string(skimT.nElectron));

    for (int eleInd = 0; eleInd < skimT.nElectron; ++eleInd) {
        double eta = skimT.Electron_eta[eleInd];
        double absEta = std::abs(eta);
        double SCeta = eta + skimT.Electron_deltaEtaSC[eleInd];
        double absSCEta = std::abs(SCeta);
        double pt = skimT.Electron_pt[eleInd];

        // Ensure it doesn't fall within the gap
        bool passEtaEBEEGap = (absSCEta < 1.4442) || (absSCEta > 1.566);
        // Tight electron ID
        bool passTightID = skimT.Electron_cutBased[eleInd] == 4;

        bool eleSel = (passEtaEBEEGap && absEta <= 2.4 && pt >= 25 && passTightID);
        if (eleSel) {
            pickedElectrons_.push_back(eleInd);
            printDebug("Electron " + std::to_string(eleInd) + " selected: pt = " + std::to_string(pt) + ", eta = " + std::to_string(eta));
        } else {
            printDebug("Electron " + std::to_string(eleInd) + " rejected: pt = " + std::to_string(pt) + ", eta = " + std::to_string(eta));
        }
    }

    printDebug("Total Electrons Selected: " + std::to_string(pickedElectrons_.size()));
}

// Photon selection
void ObjectPick::pickPhotons(const SkimTree& skimT) {
    printDebug("Starting Selection, nPhoton = "+std::to_string(skimT.nPhoton));

    for (int phoInd = 0; phoInd < skimT.nPhoton; ++phoInd) {
        double pt = skimT.Photon_pt[phoInd];
        double r9 = skimT.Photon_r9[phoInd];
        double hoe = skimT.Photon_hoe[phoInd];
        Int_t id = skimT.Photon_cutBased[phoInd];  // Tight ID
        // R9>0.94 to avoid bias wrt R9Id90 triggers and from photon conversions
        if(pt > 15 && r9 > 0.94 && r9 < 1.0 && hoe < 0.02148 && id==3){
            pickedPhotons_.push_back(phoInd);
        }
        printDebug(
            "Photon " + std::to_string(phoInd) + 
            ", Id  = " + std::to_string(id) + 
            ", pt  = " + std::to_string(pt) + 
            ", hoe  = " + std::to_string(hoe) + 
            ", r9  = " + std::to_string(r9)
       );
    }
    printDebug("Total Photons Selected: " + std::to_string(pickedPhotons_.size()));
}

// Reference object selection
void ObjectPick::pickRefs(const SkimTree& skimT) {
    // Z->ee + jets channel
    if (channel_ == GlobalFlag::Channel::ZeeJet && pickedElectrons_.size() > 1) {
        int j = pickedElectrons_.at(0);
        int k = pickedElectrons_.at(1);

        TLorentzVector p4Lep1, p4Lep2;
        double ptj = skimT.Electron_pt[j];
        double ptk = skimT.Electron_pt[k];
        p4Lep1.SetPtEtaPhiM(ptj, skimT.Electron_eta[j], skimT.Electron_phi[j], skimT.Electron_mass[j]);
        p4Lep2.SetPtEtaPhiM(ptk, skimT.Electron_eta[k], skimT.Electron_phi[k], skimT.Electron_mass[k]);
        TLorentzVector p4Ref = p4Lep1 + p4Lep2;

        if ((skimT.Electron_charge[j] * skimT.Electron_charge[k]) == -1 &&
            std::abs(p4Ref.M() - 91.1876) < 20 &&
            p4Ref.Pt() > 15) {
            pickedRefs_.push_back(p4Ref);
            printDebug("Z->ee candidate selected with mass " + std::to_string(p4Ref.M()));
        }
    }

    // Z->mumu + jets channel
    else if (channel_ == GlobalFlag::Channel::ZmmJet && pickedMuons_.size() > 1) {
        int j = pickedMuons_.at(0);
        int k = pickedMuons_.at(1);

        TLorentzVector p4Lep1, p4Lep2;
        double ptj = skimT.Muon_pt[j];
        double ptk = skimT.Muon_pt[k];
        p4Lep1.SetPtEtaPhiM(ptj, skimT.Muon_eta[j], skimT.Muon_phi[j], skimT.Muon_mass[j]);
        p4Lep2.SetPtEtaPhiM(ptk, skimT.Muon_eta[k], skimT.Muon_phi[k], skimT.Muon_mass[k]);
        TLorentzVector p4Ref = p4Lep1 + p4Lep2;

        if ((skimT.Muon_charge[j] * skimT.Muon_charge[k]) == -1 &&
            std::abs(p4Ref.M() - 91.1876) < 20 &&
            p4Ref.Pt() > 15) {
            pickedRefs_.push_back(p4Ref);
            printDebug("Z->mumu candidate selected with mass " + std::to_string(p4Ref.M()));
        }
    }

    // Gamma + jets channel
    else if (channel_ == GlobalFlag::Channel::GamJet && !pickedPhotons_.empty()) {
        for (int idx : pickedPhotons_) {
            TLorentzVector p4Pho;
            p4Pho.SetPtEtaPhiM(skimT.Photon_pt[idx], skimT.Photon_eta[idx], skimT.Photon_phi[idx], skimT.Photon_mass[idx]);
            pickedRefs_.push_back(p4Pho);
            printDebug("Photon index added to references  = " + std::to_string(idx));
        }
    }

    printDebug("Total Reference Objects Selected: " + std::to_string(pickedRefs_.size()));
}


// Jet selection
void ObjectPick::pickJets(const SkimTree& skimT, const TLorentzVector& p4Ref) {
    printDebug("Starting Selection, nJet = "+std::to_string(skimT.nJet));
    TLorentzVector p4Jeti;
    TLorentzVector p4Jet1, p4Jet2, p4Jetn;
    // Initialize jet indices and counts
    int iJet1 = -1, iJet2 = -1, nJets = 0;

    // Initialize four-momentum vectors to zero
    p4Jet1.SetPtEtaPhiM(0, 0, 0, 0);
    p4Jet2.SetPtEtaPhiM(0, 0, 0, 0);
    p4Jetn.SetPtEtaPhiM(0, 0, 0, 0);

    // First jet loop: Identify Leading and Subleading Jets Based on Selection Criteria
    for (int i = 0; i < skimT.nJet; ++i) {
        // Apply selection criteria
        if (skimT.Jet_jetId[i] < 6) continue; // TightLepVeto
        if (skimT.Jet_pt[i] < 12) continue;

        // Create a TLorentzVector for the current jet
        p4Jeti.SetPtEtaPhiM(skimT.Jet_pt[i],
                           skimT.Jet_eta[i],
                           skimT.Jet_phi[i],
                           skimT.Jet_mass[i]);

        // Check ΔR criterion
        if (p4Ref.DeltaR(p4Jeti) < 0.2) continue;
        nJets++;

        // Select Leading Jet
        if (iJet1 == -1 || p4Jeti.Pt() > p4Jet1.Pt()) {
            // Demote current leading to subleading if applicable
            if (iJet1 != -1) {
                iJet2 = iJet1;
                p4Jet2 = p4Jet1;
            }
            // Assign new leading jet
            iJet1 = i;
            p4Jet1 = p4Jeti;
            pickedJetsIndex_.push_back(iJet1);
        }
        // Select Subleading Jet
        else if (iJet2 == -1 || p4Jeti.Pt() > p4Jet2.Pt()) {
            iJet2 = i;
            p4Jet2 = p4Jeti;
            pickedJetsIndex_.push_back(iJet2);
        }
        printDebug(
            "Jet " + std::to_string(i) + 
            ", Id  = " + std::to_string(skimT.Jet_jetId[i]) + 
            ", pt  = " + std::to_string(skimT.Jet_pt[i]) + 
            ", p4Ref pT  = " + std::to_string(p4Ref.Pt())
       );
    }

    // Second Loop: Accumulate p4Jetn with All Jets Except Leading and Subleading
    for (int i = 0; i < skimT.nJet; ++i) {
        // Skip the leading and subleading jets
        if (i == iJet1 || i == iJet2) continue;

        // Create a TLorentzVector for the current jet
        p4Jeti.SetPtEtaPhiM(skimT.Jet_pt[i],
                           skimT.Jet_eta[i],
                           skimT.Jet_phi[i],
                           skimT.Jet_mass[i]);

        // Accumulate the four-momentum
        p4Jetn += p4Jeti;
    }
    pickedJetsP4_.push_back(p4Jet1);
    pickedJetsP4_.push_back(p4Jet2);
    pickedJetsP4_.push_back(p4Jetn);

    printDebug("Total Jets Selected: " + std::to_string(nJets));
}

// Gen objects
void ObjectPick::pickGenMuons(const SkimTree& skimT) {
    printDebug("Starting Selection, nGenDressedLepton = "+std::to_string(skimT.nGenDressedLepton));

    for (int i = 0; i < skimT.nGenDressedLepton; ++i) {
        if (std::abs(skimT.GenDressedLepton_pdgId[i]) == 13) {
            pickedGenMuons_.push_back(i);
            printDebug("Gen Muon " + std::to_string(i) + " selected");
        }
    }

    printDebug("Total Gen Muons Selected: " + std::to_string(pickedGenMuons_.size()));
}

void ObjectPick::pickGenElectrons(const SkimTree& skimT) {
    printDebug("Starting Selection, nGenDressedLepton = "+std::to_string(skimT.nGenDressedLepton));

    for (int i = 0; i < skimT.nGenDressedLepton; ++i) {
        if (std::abs(skimT.GenDressedLepton_pdgId[i]) == 11) {
            pickedGenElectrons_.push_back(i);
            printDebug("Gen Electron " + std::to_string(i) + " selected");
        }
    }

    printDebug("Total Gen Electrons Selected: " + std::to_string(pickedGenElectrons_.size()));
}

void ObjectPick::pickGenPhotons(const SkimTree& skimT) {
    printDebug("Starting Selection, nGenIsolatedPhoton = "+std::to_string(skimT.nGenIsolatedPhoton));

    for (int i = 0; i < skimT.nGenIsolatedPhoton; ++i) {
        pickedGenPhotons_.push_back(i);
        printDebug("Gen Photon " + std::to_string(i) + " selected");
    }

    printDebug("Total Gen Photons Selected: " + std::to_string(pickedGenPhotons_.size()));
}

void ObjectPick::pickGenRefs(const SkimTree& skimT, const TLorentzVector& p4Ref) {
    // Z->ee + jets channel
    if (channel_ == GlobalFlag::Channel::ZeeJet && pickedGenElectrons_.size() > 1) {
        for (size_t j = 0; j < pickedGenElectrons_.size(); ++j) {
            for (size_t k = j + 1; k < pickedGenElectrons_.size(); ++k) {
                TLorentzVector p4Lep1, p4Lep2;
                int idx1 = pickedGenElectrons_[j];
                int idx2 = pickedGenElectrons_[k];

                p4Lep1.SetPtEtaPhiM(skimT.GenDressedLepton_pt[idx1],
                                    skimT.GenDressedLepton_eta[idx1],
                                    skimT.GenDressedLepton_phi[idx1],
                                    skimT.GenDressedLepton_mass[idx1]);

                p4Lep2.SetPtEtaPhiM(skimT.GenDressedLepton_pt[idx2],
                                    skimT.GenDressedLepton_eta[idx2],
                                    skimT.GenDressedLepton_phi[idx2],
                                    skimT.GenDressedLepton_mass[idx2]);

                TLorentzVector p4GenRef = p4Lep1 + p4Lep2;
                if(p4GenRef.DeltaR(p4Ref) > 0.2) continue;
                pickedGenRefs_.push_back(p4GenRef);
                printDebug("Gen Z->ee candidate selected with mass " + std::to_string(p4GenRef.M()));
            }
        }
    }

    // Z->mumu + jets channel
    else if (channel_ == GlobalFlag::Channel::ZmmJet && pickedGenMuons_.size() > 1) {
        for (size_t j = 0; j < pickedGenMuons_.size(); ++j) {
            for (size_t k = j + 1; k < pickedGenMuons_.size(); ++k) {
                TLorentzVector p4Lep1, p4Lep2;
                int idx1 = pickedGenMuons_[j];
                int idx2 = pickedGenMuons_[k];

                p4Lep1.SetPtEtaPhiM(skimT.GenDressedLepton_pt[idx1],
                                    skimT.GenDressedLepton_eta[idx1],
                                    skimT.GenDressedLepton_phi[idx1],
                                    skimT.GenDressedLepton_mass[idx1]);

                p4Lep2.SetPtEtaPhiM(skimT.GenDressedLepton_pt[idx2],
                                    skimT.GenDressedLepton_eta[idx2],
                                    skimT.GenDressedLepton_phi[idx2],
                                    skimT.GenDressedLepton_mass[idx2]);

                TLorentzVector p4GenRef = p4Lep1 + p4Lep2;
                if(p4GenRef.DeltaR(p4Ref) > 0.2) continue;
                pickedGenRefs_.push_back(p4GenRef);
                printDebug("Gen Z->mumu candidate selected with mass " + std::to_string(p4GenRef.M()));
            }
        }
    }

    // Gamma + jets channel
    else  if (channel_ == GlobalFlag::Channel::GamJet && !pickedGenPhotons_.empty()) {
        for (int idx : pickedGenPhotons_) {
            TLorentzVector p4GenRef;
            p4GenRef.SetPtEtaPhiM(skimT.GenIsolatedPhoton_pt[idx],
                                  skimT.GenIsolatedPhoton_eta[idx],
                                  skimT.GenIsolatedPhoton_phi[idx],
                                  skimT.GenIsolatedPhoton_mass[idx]);
            if(p4GenRef.DeltaR(p4Ref) > 0.2) continue;
            pickedGenRefs_.push_back(p4GenRef);
            printDebug("Gen Photon added to references: pt = " + std::to_string(skimT.GenIsolatedPhoton_pt[idx]));
        }
    }

    printDebug("Total Gen Reference Objects Selected: " + std::to_string(pickedGenRefs_.size()));
}

