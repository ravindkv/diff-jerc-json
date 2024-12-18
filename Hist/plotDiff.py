#!/usr/bin/env python3

import ROOT
import json
import argparse
import os
import sys

def parse_arguments():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser(description="Plot JEC corrections from ROOT histograms based on metadata.json")
    parser.add_argument('--root_file', type=str, required=True, help='Path to the ROOT file containing histograms.')
    parser.add_argument('--metadata', type=str, required=True, help='Path to the metadata.json file.')
    parser.add_argument('--output_file', type=str, default='corrections_comparison.pdf', help='Name of the output PDF file.')
    return parser.parse_args()

def load_metadata(metadata_path):
    """
    Load and parse the metadata.json file.
    """
    if not os.path.isfile(metadata_path):
        print(f"Error: metadata.json file '{metadata_path}' not found.")
        sys.exit(1)
    with open(metadata_path, 'r') as f:
        metadata = json.load(f)
    return metadata

def get_histograms(root_file, baseKey):
    """
    Retrieve the histograms for a given baseKey from the ROOT file.
    
    Parameters:
        root_file (ROOT.TFile): The opened ROOT file.
        baseKey (str): The base key identifier.
    
    Returns:
        tuple: (hCorrV1, hCorrV2, pDiff) histograms.
    """
    hCorrV1_name = f"hCorrV1_{baseKey}"
    hCorrV2_name = f"hCorrV2_{baseKey}"
    pDiff_name = f"pDiff_{baseKey}"
    
    hCorrV1 = root_file.Get(hCorrV1_name)
    hCorrV2 = root_file.Get(hCorrV2_name)
    pDiff = root_file.Get(pDiff_name)
    
    if not hCorrV1:
        print(f"Warning: Histogram '{hCorrV1_name}' not found in ROOT file.")
    if not hCorrV2:
        print(f"Warning: Histogram '{hCorrV2_name}' not found in ROOT file.")
    if not pDiff:
        print(f"Warning: Histogram '{pDiff_name}' not found in ROOT file.")
    
    return hCorrV1, hCorrV2, pDiff

def plot_corrections(metadata, root_file, output_pdf):
    """
    Create and save plots based on the histograms and metadata.
    
    Parameters:
        metadata (dict): Parsed metadata mapping baseKeys to corrections.
        root_file (ROOT.TFile): The opened ROOT file.
        output_pdf (str): Path to the output PDF file.
    """
    # Initialize the PDF
    # Start the multi-page PDF
    c = ROOT.TCanvas("c", "JEC Corrections", 800, 800)
    c.Print(f"{output_pdf}[")

    for baseKey in metadata.keys():
        print(f"Processing baseKey: {baseKey}")
        hCorrV1, hCorrV2, pDiff = get_histograms(root_file, baseKey)
        
        # Ensure that both V1 and V2 histograms exist
        if not hCorrV1 or not hCorrV2:
            print(f"Skipping baseKey '{baseKey}' due to missing histograms.")
            continue
        
        # Create a new canvas for the main plot and ratio
        canvas = ROOT.TCanvas(f"c_{baseKey}", f"Corrections for {baseKey}", 800, 800)
        
        # Define and divide the canvas into two pads
        pad1 = ROOT.TPad("pad1", "pad1", 0, 0.3, 1, 1.0)
        pad2 = ROOT.TPad("pad2", "pad2", 0, 0.0, 1, 0.3)
        pad1.SetBottomMargin(0)  # Upper pad has no bottom margin
        pad2.SetTopMargin(0)     # Lower pad has no top margin
        pad2.SetBottomMargin(0.3)
        pad1.Draw()
        pad2.Draw()
        
        # Top pad: Plot hCorrV1 and hCorrV2
        pad1.cd()
        hCorrV1.SetLineColor(ROOT.kBlue)
        hCorrV2.SetLineColor(ROOT.kRed)
        hCorrV1.SetTitle(f"{baseKey} : V1 vs V2 Correction Factors")
        hCorrV1.GetXaxis().SetTitle("")  # No x-axis title in top pad
        hCorrV1.GetYaxis().SetTitle("Events")
        hCorrV1.SetStats(False)
        hCorrV1.Draw("HIST")
        hCorrV2.Draw("HIST SAME")
        
        # Add legend
        legend = ROOT.TLegend(0.7, 0.7, 0.9, 0.9)
        legend.AddEntry(hCorrV1, "V1", "l")
        legend.AddEntry(hCorrV2, "V2", "l")
        legend.SetBorderSize(0)
        legend.Draw()
        
        # Bottom pad: Plot the ratio hCorrV2 / hCorrV1
        pad2.cd()
        ratio = hCorrV2.Clone("ratio")
        ratio.SetTitle("")
        ratio.Divide(hCorrV1)
        ratio.SetLineColor(ROOT.kBlack)
        ratio.SetMarkerStyle(20)
        ratio.SetMarkerColor(ROOT.kBlack)
        ratio.GetXaxis().SetTitle("Correction Factor")
        ratio.GetYaxis().SetTitle("V2 / V1")
        ratio.SetStats(False)
        ratio.Draw("E")
        
        # Draw a horizontal line at y=1 for reference
        line = ROOT.TLine(ratio.GetXaxis().GetXmin(), 1.0, ratio.GetXaxis().GetXmax(), 1.0)
        line.SetLineColor(ROOT.kGray)
        line.SetLineStyle(2)
        line.Draw("same")
        
        # Save the main canvas to the PDF
        canvas.Print(output_pdf)
        
        # Now plot pDiff if it exists
        if pDiff:
            # Create another canvas for pDiff
            pDiff_canvas = ROOT.TCanvas(f"c_pDiff_{baseKey}", f"Difference for {baseKey}", 800, 600)
            pDiff.SetTitle(f"{baseKey} : 100*(V1 - V2)/V1 vs pT")
            pDiff.GetXaxis().SetTitle("Jet p_{T} [GeV]")
            pDiff.GetYaxis().SetTitle("Difference in % (100 x (V1 - V2)/V1)")
            #pDiff.GetYaxis().SetRangeUser(-0.5, 0.5);
            pDiff.SetMarkerStyle(20)
            pDiff.SetMarkerColor(ROOT.kGreen+2)
            pDiff.SetLineColor(ROOT.kGreen+2)
            pDiff.SetStats(False)
            pDiff.Draw("P")
            
            # Save pDiff plot to the PDF
            pDiff_canvas.Print(output_pdf)
        
        # Cleanup
        ROOT.SetOwnership(canvas, False)       # Avoid ROOT taking ownership
        ROOT.SetOwnership(pDiff_canvas, False) # Avoid ROOT taking ownership
        del canvas
        del pDiff_canvas
        del ratio
        del line
        del legend
    
    # Close the multi-page PDF
    c.Print(f"{output_pdf}]")
    c.Close()

def main():
    """
    Main function to execute the plotting workflow.
    """
    args = parse_arguments()
    
    # Load metadata.json
    metadata = load_metadata(args.metadata)
    
    # Open the ROOT file
    if not os.path.isfile(args.root_file):
        print(f"Error: ROOT file '{args.root_file}' not found.")
        sys.exit(1)
    
    root_file = ROOT.TFile.Open(args.root_file, "READ")
    if not root_file or root_file.IsZombie():
        print(f"Error: Unable to open ROOT file '{args.root_file}'.")
        sys.exit(1)
    
    # Plot corrections and save to PDF
    plot_corrections(metadata, root_file, args.output_file)
    
    # Close the ROOT file
    root_file.Close()
    print(f"All plots saved to '{args.output_file}'.")

if __name__ == "__main__":
    main()

