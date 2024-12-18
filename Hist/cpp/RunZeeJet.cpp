#include "RunZeeJet.h"

#include "Helper.h"
#include "FillHist.h"
#include "nlohmann/json.hpp"
   
// Constructor implementation
RunZeeJet::RunZeeJet(GlobalFlag& globalFlags)
    :globalFlags_(globalFlags) {
}

auto RunZeeJet::Run(std::shared_ptr<SkimTree>& skimT, EventPick *eventP, ObjectPick *objP, ScaleObject *scaleObject, const std::string& metadataJsonPath, TFile *fout) -> int{
   
    assert(fout && !fout->IsZombie());
    fout->cd();
 
    TDirectory *origDir = gDirectory;
    //------------------------------------
    // Initialise hists and directories 
    //------------------------------------
    FillHist fillHist(metadataJsonPath, 100, 0.0, 1000.0);
    fillHist.initialize();
    
    // Parse the metadata JSON
    std::ifstream inFile(metadataJsonPath);
    nlohmann::json meta;
    inFile >> meta;
    inFile.close();


    double totalTime = 0.0;
    auto startClock = std::chrono::high_resolution_clock::now();
    Long64_t nentries = skimT->getEntries();
    Helper::initProgress(nentries);

    for (Long64_t jentry = 0; jentry < nentries; ++jentry) {
        if (globalFlags_.isDebug() && jentry > globalFlags_.getNDebug()) break;
        Helper::printProgress(jentry, nentries, startClock, totalTime);
       
        Long64_t ientry = skimT->loadEntry(jentry);
        if (ientry < 0) break; 
        //if (ientry > 10000) break; 
        skimT->getChain()->GetTree()->GetEntry(ientry);

        for (int i = 0; i < skimT->nJet; ++i) {
            if (skimT->Jet_jetId[i] < 6) continue; // TightLepVeto
            if (skimT->Jet_pt[i] < 10) continue;
        
            std::vector<double> corrFactors = {0.0, 0.0};
            std::vector<double> inputs = {};
            for (auto it = meta.begin(); it != meta.end(); ++it) {
                std::string baseKey = it.key();  
                if (baseKey.find("_ScaleFactor_") != std::string::npos){
                    corrFactors = scaleObject->evaluateJerSF(baseKey, skimT->Jet_eta[i], skimT->Jet_pt[i],"nom");
                }
                else {
                    if (baseKey.find("_L1FastJet_") != std::string::npos){
                        inputs  = {skimT->Jet_area[i],skimT->Jet_eta[i], skimT->Jet_pt[i], skimT->Rho};
                    }
                    else if (baseKey.find("_L2Relative_") != std::string::npos){
                        inputs  = {skimT->Jet_eta[i], skimT->Jet_phi[i], skimT->Jet_pt[i]};
                    }
                    else if (baseKey.find("_L3Absolute_") != std::string::npos){
                        inputs  = {skimT->Jet_eta[i], skimT->Jet_pt[i]};
                    }
                    else if (baseKey.find("_L2L3Residual_") != std::string::npos){
                        inputs  = {skimT->Jet_eta[i], skimT->Jet_pt[i]};
                    }
                    else if (baseKey.find("_PtResolution_") != std::string::npos){
                        inputs  = {skimT->Jet_eta[i], skimT->Jet_pt[i], skimT->Rho};
                    }
                    else {
                        inputs  = {skimT->Jet_eta[i], skimT->Jet_pt[i]};
                    }
                    corrFactors = scaleObject->evaluateCorrections(baseKey, inputs);
                }
                fillHist.fill(baseKey,  skimT->Jet_pt[i], corrFactors);
            }//metadata loop
        }//jet loop
    }//event loop
    fout->Write();
    //Helper::scanTFile(fout);
    std::cout << "Output file: " << fout->GetName() << '\n';
    return 0;
}
   
