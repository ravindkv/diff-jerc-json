#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TH1.h>
#include <TH2D.h>
#include <TH2F.h>
#include <TProfile2D.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>
#include <TLatex.h>
#include <iostream>
#include <string>
#include <TList.h>
#include <TClass.h>
#include <TGraphErrors.h>
#include <TStyle.h>
#include <TROOT.h>
#include <iomanip>

//$ g++ -o /afs/cern.ch/user/r/rverma/public/diffTFiles diffTFiles.C `root-config --cflags --glibs`
// ./diffTFiles file1.root file2.root

// General templated function to calculate ratio and fill a TGraphErrors
template<typename T>
void calculateHistRatio(T* dataHist, T* mcHist, TGraphErrors* ratioGraph) {
    int nBins = dataHist->GetNbinsX();
    for (int i = 1; i <= nBins; ++i) {
        double dataVal = dataHist->GetBinContent(i);
        double mcVal = mcHist->GetBinContent(i);
        double ratioVal = 0.0;
        double ratioErr = 0.0;
        double binWidth = dataHist->GetXaxis()->GetBinWidth(i);  // Get the bin width
        if(mcVal > 0 && dataVal > 0){
            ratioVal = dataVal / mcVal;
            ratioErr = sqrt(pow(dataHist->GetBinError(i) / mcVal, 2) +
                            pow(dataVal * mcHist->GetBinError(i) / pow(mcVal, 2), 2));
        }
        ratioGraph->SetPoint(i - 1, dataHist->GetXaxis()->GetBinCenter(i), ratioVal);
        ratioGraph->SetPointError(i - 1, binWidth / 2.0, ratioErr);  // Set x-error to half the bin width
    }
}


void compareHistograms(TH1* h1, TH1* h2, const std::string& objPath, const char* pdfFileName, TCanvas* c1);
void compareDirectories(TDirectory* dir1, TDirectory* dir2, const std::string& path, const char* pdfFileName, TCanvas* c1, const char* file1name, const char* file2name);

void compareRootFiles(const char* file1name, const char* file2name) {
    TFile* file1 = TFile::Open(file1name);
    TFile* file2 = TFile::Open(file2name);
    
    if (!file1 || !file2) {
        std::cerr << "Error opening files." << std::endl;
        return;
    }
    
    // Output PDF file
    const char* pdfFileName = "comparison.pdf";
    // Start the multi-page PDF
    TCanvas* c1 = new TCanvas("c1", "", 800, 800);
    c1->Print(Form("%s[", pdfFileName)); // This starts the multi-page PDF
    
    compareDirectories(file1, file2, "", pdfFileName, c1, file1name, file2name);
    
    c1->Print(Form("%s]", pdfFileName)); // This ends the multi-page PDF
    
    delete c1;
    
    file1->Close();
    file2->Close();
}

void compareDirectories(TDirectory* dir1, TDirectory* dir2, const std::string& path, const char* pdfFileName, TCanvas* c1, const char* file1name, const char* file2name) {

    TList* keys = dir1->GetListOfKeys();
    TIter nextkey(keys);
    TKey* key;
     
    while ((key = (TKey*)nextkey())) {
        TObject* obj1 = key->ReadObj();
        const char* name = obj1->GetName();
        
        // Build the path to the object
        std::string objPath = path.empty() ? name : path + "/" + name;
        
        // Try to get the object from dir2
        TObject* obj2 = dir2->Get(name);
        if (!obj2) {
            std::cout << "Object " << objPath << " not found in second file." << std::endl;
            continue;
        }
        
        // Check if it's a directory
        if (obj1->InheritsFrom(TDirectory::Class())) {
            // It's a directory, recurse
            TDirectory* subdir1 = (TDirectory*)obj1;
            TDirectory* subdir2 = (TDirectory*)obj2;
            compareDirectories(subdir1, subdir2, objPath, pdfFileName, c1, file1name, file2name);
        }
        else if (obj1->InheritsFrom(TH1::Class())) {
            if (TProfile2D *prof2d = dynamic_cast<TProfile2D *>(obj1)) continue;
            if (TH2D *th2d = dynamic_cast<TH2D *>(obj1)) continue;
            if (TH2F *th2f = dynamic_cast<TH2F *>(obj1)) continue;
            // It's a histogram, compare
            TH1* h1 = (TH1*)obj1->Clone(file1name);
            TH1* h2 = (TH1*)obj2->Clone(file2name);
            compareHistograms(h1, h2, objPath, pdfFileName, c1);
        }
        else {
            std::cout << "Object " << objPath << " is of unsupported type." << std::endl;
        }
    }
}

void compareHistograms(TH1* h1, TH1* h2, const std::string& objPath, const char* pdfFileName, TCanvas* c1) {
    gStyle->SetOptStat(0);
    c1->Clear();
    
    // Create upper and lower pads
    TPad* pad1 = new TPad("pad1", "pad1", 0, 0.3, 1, 1);
    pad1->SetBottomMargin(0.02);
    pad1->SetLogx(true);
    pad1->SetLogy(true);
    pad1->Draw();
    TPad* pad2 = new TPad("pad2", "pad2", 0, 0, 1, 0.3);
    pad2->SetTopMargin(0.02);
    pad2->SetBottomMargin(0.3);
    pad2->SetLogx(true);
    pad2->Draw();
    
    // Upper pad
    pad1->cd();
    h1->SetLineColor(kRed);
    h1->SetLineWidth(2);
    h1->GetYaxis()->CenterTitle();
    h1->GetXaxis()->SetTitleOffset(1.0);
    h1->GetYaxis()->SetTitleOffset(1.15);
    h1->GetXaxis()->SetTitleSize(0.05);
    h1->GetYaxis()->SetTitleSize(0.07);
    h1->GetXaxis()->SetLabelSize(0.05);
    h1->GetYaxis()->SetLabelSize(0.05);
    h1->GetXaxis()->SetMoreLogLabels();

    h2->SetLineColor(kBlue);
    h2->SetLineWidth(2);
    h2->GetYaxis()->CenterTitle();
    h2->GetXaxis()->SetTitleOffset(1.0);
    h2->GetYaxis()->SetTitleOffset(1.15);
    h2->GetXaxis()->SetTitleSize(0.05);
    h2->GetYaxis()->SetTitleSize(0.07);
    h2->GetXaxis()->SetLabelSize(0.05);
    h2->GetYaxis()->SetLabelSize(0.05);
    h2->GetXaxis()->SetMoreLogLabels();
    
    h1->Draw("hist");
    h2->Draw("hist same");
    
    // Add legend
 	TLegend* leg = new TLegend(0.2, 0.6, 0.9, 0.9);
    leg->SetFillStyle(kNone);
    leg->SetBorderSize(0);
    leg->SetTextSize(0.040);

    // Retrieve statistical information for the first histogram with rounded values
    std::ostringstream h1StatsStream;
    h1StatsStream << "Entries: " << static_cast<int>(h1->GetEntries()) 
                  << ", Mean: " << std::fixed << std::setprecision(1) << h1->GetMean() 
                  << ", RMS: " << std::fixed << std::setprecision(1) << h1->GetRMS(); 
    std::string h1Stats = h1StatsStream.str();

    // Add the first entry (name) and second entry (stats) for h1
    leg->AddEntry(h1, h1->GetName(), "l");
    leg->AddEntry((TObject*)0, h1Stats.c_str(), "");

    // Retrieve statistical information for the second histogram with rounded values
    std::ostringstream h2StatsStream;
    h2StatsStream << "Entries: " << static_cast<int>(h2->GetEntries()) 
                  << ", Mean: " << std::fixed << std::setprecision(1) << h2->GetMean() 
                  << ", RMS: " << std::fixed << std::setprecision(1) << h2->GetRMS(); 
    std::string h2Stats = h2StatsStream.str();

    // Add the first entry (name) and second entry (stats) for h2
    leg->AddEntry(h2, h2->GetName(), "l");
    leg->AddEntry((TObject*)0, h2Stats.c_str(), "");


	leg->Draw();
 
    
    // Add title
    TLatex* latex = new TLatex();
    latex->SetNDC();
    latex->SetTextSize(0.04);
    latex->DrawLatex(0.1, 0.92, objPath.c_str());
    
    // Lower pad
    pad2->cd();
    
    TGraphErrors* graphRatio = new TGraphErrors(h1->GetNbinsX());
    calculateHistRatio(h1, h2, graphRatio);
    
	graphRatio->GetHistogram()->SetTitle("");
    // X-axis styling
    graphRatio->GetHistogram()->GetXaxis()->SetTitleSize(0.12);
    graphRatio->GetHistogram()->GetXaxis()->SetLabelSize(0.12);
    graphRatio->GetHistogram()->GetXaxis()->SetLabelFont(42);
    graphRatio->GetHistogram()->GetXaxis()->SetTitleOffset(1.2);
    graphRatio->GetHistogram()->GetXaxis()->SetLabelOffset(0.01);

    // Y-axis styling
    graphRatio->GetHistogram()->GetYaxis()->SetTitleSize(0.13);
    graphRatio->GetHistogram()->GetYaxis()->SetLabelSize(0.12);
    graphRatio->GetHistogram()->GetYaxis()->SetLabelFont(42);
    graphRatio->GetHistogram()->GetYaxis()->SetNdivisions(6, 5, 0);
    graphRatio->GetHistogram()->GetYaxis()->SetTitleOffset(0.6);
    graphRatio->GetHistogram()->GetYaxis()->SetLabelOffset(0.01);
    graphRatio->GetHistogram()->GetYaxis()->CenterTitle();

    // Additional styling
    graphRatio->SetMarkerStyle(20);  // Set marker style for points

    // Optional: Log scale or no exponent for x-axis
    graphRatio->GetHistogram()->GetXaxis()->SetMoreLogLabels();
    graphRatio->GetHistogram()->GetXaxis()->SetNoExponent();
    graphRatio->GetHistogram()->GetYaxis()->SetRangeUser(0.9, 1.1);
    
    graphRatio->Draw("APz");
    
    // Save the canvas to the PDF file
    c1->Print(pdfFileName);
    
    // Clean up
    delete graphRatio;
    delete leg;
    delete latex;
    // Note: do not delete pads, they are owned by the canvas
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: compareRootFiles file1.root file2.root" << std::endl;
        return 1;
    }
    gROOT->SetBatch(true); 
    compareRootFiles(argv[1], argv[2]);
    
    return 0;
}

