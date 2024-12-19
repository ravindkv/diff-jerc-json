
#include "Helper.h"
#include "TTree.h"
#include "TKey.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TMath.h"

double Helper::DELTAPHI(double phi1, double phi2) {
  double dphi = fabs(phi1 - phi2);
  return (dphi <= TMath::Pi() ? dphi : TMath::TwoPi() - dphi);
}

double Helper::DELTAR(double phi1, double phi2, double eta1, double eta2){
  return sqrt(pow(DELTAPHI(phi1, phi2), 2) + pow(eta1 - eta2, 2));
}
 

std::vector<std::vector<std::string>> Helper::splitVector(const std::vector<std::string>& strings, int n) {
    int size = strings.size() / n;  // Size of each small vector
    int remainder = strings.size() % n;  // Remaining elements
    std::vector<std::vector<std::string>> smallVectors;
    int index = 0;
    for (int i = 0; i < n; ++i) {
        if (i < remainder) {
            smallVectors.push_back(std::vector<std::string>(
                        strings.begin() + index, strings.begin() + index + size + 1));
            index += size + 1;
        } else {
            smallVectors.push_back(std::vector<std::string>(
                        strings.begin() + index, strings.begin() + index + size));
            index += size;
        }
    }
    return smallVectors;
}

std::vector<std::string> Helper::splitString(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    
    while ((end = s.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(s.substr(start)); // Last token
    
    return tokens;
}

// Function to print progress during event processing
void Helper::initProgress(Long64_t nentries){
    std::cout << "\nStarting loop over " << nentries << " entries" << '\n';
    std::cout<<"---------------------------"<<'\n';
    std::cout<<std::setw(10)
             <<"Progress"<<std::setw(10)
             <<"Time"
             <<'\n';
    std::cout<<"---------------------------"<<'\n';
}

void Helper::printProgress(Long64_t jentry, Long64_t nentries,
                              std::chrono::time_point<std::chrono::high_resolution_clock>& startClock,
                              double& totTime){
    bool isDebug_ = false;
    if (isDebug_) {
        std::cout << "\n=== Event: " << jentry << " ===\n" << '\n';
    }
    if (nentries > 100 && jentry % (nentries / 100) == 0) {  // Print progress every 1%
        auto currentTime = std::chrono::high_resolution_clock::now();
        totTime += std::chrono::duration<double>(currentTime - startClock).count();
        int sec = static_cast<int>(totTime) % 60;
        int min = static_cast<int>(totTime) / 60;
        std::cout << std::setw(5) << (100 * jentry / nentries) << "% "
                  << std::setw(5) << min << "m " << sec << "s" << '\n';
        startClock = currentTime;  // Reset clock after printing progress
    }
}

// Function to print information about ROOT objects
void Helper::printInfo(const TObject* obj){
    if (const auto* tree = dynamic_cast<const TTree*>(obj)) {
        std::cout << std::setw(15) << "TTree: " << std::setw(35) << tree->GetName()
                  << std::setw(15) << tree->GetEntries() << '\n';
    } else if (const auto* prof = dynamic_cast<const TProfile*>(obj)) {
        std::cout << std::setw(15) << "TProfile: " << std::setw(35) << prof->GetName()
                  << std::setw(15) << prof->GetEntries()
                  << std::setw(15) << prof->GetMean()
                  << std::setw(15) << prof->GetRMS() << '\n';
    } else if (const auto* prof2d = dynamic_cast<const TProfile2D*>(obj)) {
        std::cout << std::setw(15) << "TProfile2D: " << std::setw(35) << prof2d->GetName()
                  << std::setw(15) << prof2d->GetEntries()
                  << std::setw(15) << prof2d->GetMean()
                  << std::setw(15) << prof2d->GetRMS() << '\n';
    } else if (const TH1* hist = dynamic_cast<const TH1*>(obj)) {
        std::cout << std::setw(15) << hist->ClassName() << ": " << std::setw(35) << hist->GetName()
                  << std::setw(15) << hist->GetEntries()
                  << std::setw(15) << hist->GetMean()
                  << std::setw(15) << hist->GetRMS() << '\n';
    } else {
        std::cout << std::setw(15) << obj->ClassName() << ": " << std::setw(35) << obj->GetName()
                  << '\n';
    }
}

// Function to scan a directory and its contents recursively
void Helper::scanDirectory(TDirectory* dir, const std::string& path){
    std::string currentPath = path + dir->GetName() + "/";
    std::cout << "\nDirectory: " << currentPath << '\n';

    TIter next(dir->GetListOfKeys());
    TKey* key = nullptr;

    while ((key = dynamic_cast<TKey*>(next()))) {
        TObject* obj = key->ReadObj();

        if (obj->InheritsFrom(TDirectory::Class())) {
            scanDirectory(dynamic_cast<TDirectory*>(obj), currentPath);  // Recursive call for subdirectories
        } else {
            printInfo(obj);
        }
    }
}

// Function to scan a ROOT file and its directories
void Helper::scanTFile(TFile* file){
    std::cout << "\n-----------: Scanning All Directories and Printing Entries, Mean, RMS :------------\n" << '\n';
    scanDirectory(file, "");
}

TDirectory* Helper::createTDirectory(TDirectory* origDir, const std::string& directoryPath) {
    if (!origDir) {
        throw std::invalid_argument("Helper::GetOrCreateDirectory - Invalid directory pointer provided.");
    }

    // Start from the original directory
    TDirectory* currentDir = origDir;

    // Split the directoryPath into components using '/'
    std::stringstream ss(directoryPath);
    std::string dirName;
    while (std::getline(ss, dirName, '/')) {
        // Check if the subdirectory exists under the current directory
        TDirectory* subDir = currentDir->GetDirectory(dirName.c_str());
        if (!subDir) {
            // Create the subdirectory if it doesn't exist
            subDir = currentDir->mkdir(dirName.c_str());
            if (!subDir) {
                throw std::runtime_error("Helper::GetOrCreateDirectory - Failed to create directory: " + dirName + " in path " + currentDir->GetPath());
            }
        }
        // Navigate into the subdirectory
        currentDir = subDir;
    }

    return currentDir; // The final directory
}

// Utility function to format numbers
std::string Helper::formatNumber(double num) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << num; // One decimal place
    std::string formatted = oss.str();
    // Replace '.' with 'p' and remove trailing '.0'
    std::replace(formatted.begin(), formatted.end(), '.', 'p');
    if (formatted.back() == 'p') {
        formatted.pop_back(); // Remove trailing 'p'
    }
    return formatted;
}
