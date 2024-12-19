#include "RunZeeJet.h"

#include "Helper.h"
#include "HistGivenPt.h"
#include "HistGivenEta.h"
#include "HistGivenBoth.h"
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
    // Define pT and eta bin edges
    //------------------------------------
    const int nPtBins = 6;
    const double ptBinEdges[nPtBins + 1] = {15, 30, 50, 110, 500, 1000, 4500};
    
    const int nEtaBins = 4;
    const double etaBinEdges[nEtaBins + 1] = {0.0, 1.3, 2.5, 3.0, 5.0};
    
    //------------------------------------
    // Initialize Hists 
    //------------------------------------
    // Create a 1D vector to hold unique_ptrs to HistGivenPt objects
    std::vector<std::unique_ptr<HistGivenPt>> histGivenPts;
    for(int ptBin = 0; ptBin < nPtBins; ++ptBin){
        std::string dirName = "Pt_" + Helper::formatNumber(ptBinEdges[ptBin]) + 
                              "_" + Helper::formatNumber(ptBinEdges[ptBin+1]);
        histGivenPts.emplace_back(std::make_unique<HistGivenPt>(origDir, dirName, metadataJsonPath));
    }

    // Create a 1D vector to hold unique_ptrs to HistGivenEta objects
    std::vector<std::unique_ptr<HistGivenEta>> histGivenEtas;
    for(int etaBin = 0; etaBin < nEtaBins; ++etaBin){
        std::string dirName = "Eta_" + Helper::formatNumber(etaBinEdges[etaBin]) + 
                              "_" + Helper::formatNumber(etaBinEdges[etaBin+1]);
        histGivenEtas.emplace_back(std::make_unique<HistGivenEta>(origDir, dirName, metadataJsonPath));
    }

    // Create a 2D vector to hold unique_ptrs to HistGivenBoth objects
    std::vector<std::vector<std::unique_ptr<HistGivenBoth>>> histGivenBoths;
    histGivenBoths.reserve(nEtaBins); // Reserve space for outer vector

    for(int etaBin = 0; etaBin < nEtaBins; ++etaBin){
        std::vector<std::unique_ptr<HistGivenBoth>> ptHists;
        ptHists.reserve(nPtBins); // Reserve space for inner vector

        for(int ptBin = 0; ptBin < nPtBins; ++ptBin){
            // Create a unique name for each HistGivenBoth based on bin ranges
            std::string histName = "Eta_" + Helper::formatNumber(etaBinEdges[etaBin]) + 
                                    "_" + Helper::formatNumber(etaBinEdges[etaBin+1]) + 
                                    "_Pt_" + Helper::formatNumber(ptBinEdges[ptBin]) + 
                                    "_" + Helper::formatNumber(ptBinEdges[ptBin+1]);
            // Initialize HistGivenBoth for this bin
            ptHists.emplace_back(std::make_unique<HistGivenBoth>(origDir, histName, metadataJsonPath));
        }

        histGivenBoths.emplace_back(std::move(ptHists)); // Move the inner vector into the outer vector
    }
    
    //------------------------------------
    // Parse the metadata JSON
    //------------------------------------
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
                
                // Determine eta bin
                int etaBin = -1;
                double absEta = std::abs(skimT->Jet_eta[i]);
                for(int b = 0; b < nEtaBins; ++b){
                    if(absEta >= etaBinEdges[b] && absEta < etaBinEdges[b+1]){
                        etaBin = b;
                        break;
                    }
                }
                // Handle edge case where eta == upper edge
                if(etaBin == -1 && absEta == etaBinEdges[nEtaBins]){
                    etaBin = nEtaBins - 1;
                }

                // Determine pT bin
                int ptBin = -1;
                double pt = skimT->Jet_pt[i];
                for(int b = 0; b < nPtBins; ++b){
                    if(pt >= ptBinEdges[b] && pt < ptBinEdges[b+1]){
                        ptBin = b;
                        break;
                    }
                }
                // Handle edge case where pt == upper edge
                if(ptBin == -1 && pt == ptBinEdges[nPtBins]){
                    ptBin = nPtBins - 1;
                }

                // If the jet falls outside the defined bins, skip filling
                if(etaBin == -1 || ptBin == -1){
                    continue;
                }

                // Fill the corresponding histogram
                histGivenPts[ptBin]->fill(baseKey, skimT->Jet_eta[i], corrFactors);

                histGivenEtas[etaBin]->fill(baseKey, skimT->Jet_pt[i], corrFactors);

                histGivenBoths[etaBin][ptBin]->fill(baseKey, corrFactors);
            }//metadata loop
        }//jet loop
    }//event loop

    fout->Write();
    //Helper::scanTFile(fout);
    std::cout << "Output file: " << fout->GetName() << '\n';
    return 0;
}
   
