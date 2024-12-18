#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TTree.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <TROOT.h>
#include <algorithm>

using namespace std;

//$ g++ -o /afs/cern.ch/user/r/rverma/public/scanTFile scanTFile.C `root-config --cflags --glibs` 

//$ ./scanTFile file.root [deep]

void printASCIIHistogram(TH1 *h1, int maxBins = 50) {
    if (h1 == nullptr) {
        std::cerr << "Error: Null histogram pointer provided.\n";
        return;
    }

    int nbinsX = h1->GetNbinsX();
    int binsToPrint = std::min(nbinsX, maxBins);
    double maxContent = h1->GetMaximum();

    // Vector to store bins with NaN content
    std::vector<std::string> nanBins;

    // Header
    std::cout << std::setw(7) << "Bin" 
              << std::setw(12) << "Center" 
              << " | Content" << '\n';
    std::cout << "----------------------------------------\n";

    // Print each bin
    for (int bin = 1; bin <= binsToPrint; ++bin) {
        double content = h1->GetBinContent(bin);
        double center  = h1->GetBinCenter(bin);
        int barLength = 0;
        bool isNan = false;

        if (std::isnan(content)) {
            isNan = true;
            nanBins.emplace_back("Bin " + std::to_string(bin));
        } else if (maxContent > 0) {
            barLength = static_cast<int>(50.0 * content / maxContent);
            // Ensure barLength does not exceed 50
            barLength = std::min(barLength, 50);
        }

        // Print bin number, center, and bar
        std::cout << std::setw(7) << bin 
                  << std::setw(12) << std::fixed << std::setprecision(2) << center 
                  << " | ";

        if (isNan) {
            std::cout << "NaN";
        } else {
            for (int i = 0; i < barLength; ++i) {
                std::cout << "*";
            }
            // Align content to the right
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << content;
        }
        std::cout << std::endl;
    }

    // Separator before underflow and overflow
    std::cout << "----------------------------------------\n";

    // Function to process underflow and overflow
    auto processSpecialBin = [&](const std::string& label, double content) -> void {
        int barLength = 0;
        bool isNan = false;

        if (std::isnan(content)) {
            isNan = true;
            nanBins.emplace_back(label + "flow");
        } else if (maxContent > 0) {
            barLength = static_cast<int>(50.0 * content / maxContent);
            barLength = std::min(barLength, 50);
        }

        std::cout << std::setw(7) << label 
                  << std::setw(12) << "N/A" 
                  << " | ";

        if (isNan) {
            std::cout << "NaN";
        } else {
            for (int i = 0; i < barLength; ++i) {
                std::cout << "*";
            }
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << content;
        }
        std::cout << std::endl;
    };

    // Print Underflow
    double underContent = h1->GetBinContent(0); // Underflow bin
    processSpecialBin("Under", underContent);

    // Print Overflow
    double overContent = h1->GetBinContent(nbinsX + 1); // Overflow bin
    processSpecialBin("Over", overContent);

    // Print summary of NaN bins, if any
    if (!nanBins.empty()) {
        std::cout << "\nWarning: The following bins contain NaN values:\n";
        for (const auto& binName : nanBins) {
            std::cout << "  - " << binName << "\n";
        }
    }
}

void printGridHistogram(TH2 *h2, int maxBinsX = 10, int maxBinsY = 10) {
    int nbinsX = h2->GetNbinsX();
    int nbinsY = h2->GetNbinsY();
    int binsXToPrint = std::min(nbinsX, maxBinsX);
    int binsYToPrint = std::min(nbinsY, maxBinsY);

    std::cout << "\nGrid Histogram: " << h2->GetName() << std::endl;

    // Print X axis labels
    std::cout << "            ";
    for (int binx = 1; binx <= binsXToPrint; ++binx) {
        std::cout << std::setw(8) << binx;
    }
    std::cout << std::endl;

    std::cout << "            ";
    for (int binx = 1; binx <= binsXToPrint; ++binx) {
		double binCenterX = h2->GetXaxis()->GetBinCenter(binx);
        std::cout << std::setw(8) << binCenterX;
    }
    std::cout << std::endl;

    // Print Y axis labels and bin contents
    for (int biny = binsYToPrint; biny >= 1; --biny) {
		double binCenterY = h2->GetYaxis()->GetBinCenter(biny);
        std::cout << std::setw(3) << biny <<std::setw(8) <<binCenterY << " | ";
        for (int binx = 1; binx <= binsXToPrint; ++binx) {
            double content = h2->GetBinContent(binx, biny);
            // Format output based on content value
            if (content == 0.0) {
                std::cout << std::setw(8) << std::fixed << std::setprecision(0) << content;
            }else if (content < 1.0) {
                std::cout << std::setw(8) << std::fixed << std::setprecision(2) << content;
            } else {
                std::cout << std::setw(8) << std::fixed << std::setprecision(1) << content;
            }
        }
        std::cout << std::endl;
    }
}


void printInfo(TObject *obj, bool deep = false) {
    if (TTree *tree = dynamic_cast<TTree *>(obj)) {
        std::cout << setw(15) << "TTree: " << setw(35) << tree->GetName()
                  << setw(15) << tree->GetEntries() << std::endl;
    } else if (TProfile *prof = dynamic_cast<TProfile *>(obj)) {
        std::cout << setw(15) << "TProfile: " << setw(35) << prof->GetName()
                  << setw(15) << prof->GetEntries() << setw(15) << prof->GetMean()
                  << setw(15) << prof->GetRMS() << std::endl;
        if (deep) {
            printASCIIHistogram(prof);
        }
    } else if (TProfile2D *prof2d = dynamic_cast<TProfile2D *>(obj)) {
        std::cout << setw(15) << "TProfile2D: " << setw(35) << prof2d->GetName()
                  << setw(15) << prof2d->GetEntries() << setw(15) << prof2d->GetMean()
                  << setw(15) << prof2d->GetRMS() << std::endl;
        if (deep) {
            printGridHistogram(prof2d);
        }
    } else if (TH1 *h1 = dynamic_cast<TH1 *>(obj)) {
        std::cout << setw(15) << h1->ClassName() << ": " << setw(35) << h1->GetName()
                  << setw(15) << h1->GetEntries() << setw(15) << h1->GetMean()
                  << setw(15) << h1->GetRMS() << std::endl;
        if (deep) {
            int dim = h1->GetDimension();
            if (dim == 1) {
                printASCIIHistogram(h1);
            } else if (dim == 2) {
                TH2 *h2 = dynamic_cast<TH2 *>(h1);
                if (h2) {
                    printGridHistogram(h2);
                } else {
                    std::cout << "Error: Histogram is 2D but cannot be cast to TH2."
                              << std::endl;
                }
            } else {
                std::cout << "3D histograms are not supported for visual representation."
                          << std::endl;
            }
        }
    } else {
        // Other object types
        std::cout << setw(15) << obj->ClassName() << ": " << setw(35) << obj->GetName()
                  << std::endl;
    }
}

void scanDirectory(TDirectory *dir, const std::string &path = "", bool deep = false) {
    std::string currentPath = path + dir->GetName() + "/";
    std::cout << "\nDirectory: " << currentPath << std::endl;

    TIter next(dir->GetListOfKeys());
    TKey *key;

    while ((key = (TKey *)next())) {
        TObject *obj = key->ReadObj();

        if (obj->InheritsFrom(TDirectory::Class())) {
            scanDirectory((TDirectory *)obj, currentPath, deep); // Recursive call
        } else {
            if(deep)std::cout<<'\n';
            printInfo(obj, deep);
        }
    }
}

int main(int argc, char **argv) {
    gROOT->SetBatch(kTRUE);
    if (argc < 2 || argc > 3) {
        std::cout << "Usage: scanTFile file.root [deep]" << std::endl;
        return 1;
    }
    bool deep = false;
    if (argc == 3 && std::string(argv[2]) == "deep") {
        deep = true;
    }
    TFile *file = TFile::Open(argv[1]);
    if (!file || file->IsZombie()) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }
    cout << "\n-----------: Scan all directories and print Entries, Mean, RMS :------------\n"
         << endl;
    scanDirectory(file, "", deep);

    file->Close();
    delete file;
    return 0;
}

