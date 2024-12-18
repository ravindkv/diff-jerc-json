#pragma once

#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include "Rtypes.h"
#include "TH1D.h"
#include "TFile.h"
#include "TDirectory.h"
#include <cmath>

class Helper {
public:
    static double DELTAPHI(double phi1, double phi2);
    static double DELTAR(double phi1, double phi2, double eta1, double eta2);

    //Function to split a vector into smallare vectors
    static std::vector<std::vector<std::string>> splitVector(const std::vector<std::string>& strings, int n);
    
    // Function to split a string into smaller vectors of string 
    static std::vector<std::string> splitString(const std::string& s, const std::string& delimiter);
    
    //Function to print the progress time of an event loop
    static void initProgress(Long64_t nentries); 
    static void printProgress(Long64_t jentry, Long64_t nentries, 
                       std::chrono::time_point<std::chrono::high_resolution_clock>& startClock, 
                       double& totTime);

    // Functions to scan a root file and print infor for TTree, TH1, etc 
    static void printInfo(const TObject* obj);
    static void scanDirectory(TDirectory* dir, const std::string& path);
    static void scanTFile(TFile* file);

    // Method to create or get nested directories
    static TDirectory* createTDirectory(TDirectory* origDir, const std::string& directoryPath);

};
    
#endif // HELPER_H

