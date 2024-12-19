import ROOT
import os
import sys
import math
import array

def get_grid_dimensions(n):
    if n == 0:
        return (1,1)
    cols = math.floor(math.sqrt(n))
    rows = math.ceil(n / cols)
    while cols * rows < n:
        cols += 1
        rows = math.ceil(n / cols)
    return cols, rows

def compute_ratio_graph(pCorrV1, pCorrV2):
    n_bins = pCorrV1.GetNbinsX()
    x_vals = []
    y_vals = []
    x_errs = []
    y_errs = []

    for bin in range(1, n_bins + 1):
        x = pCorrV1.GetBinCenter(bin)
        x_err = pCorrV1.GetBinWidth(bin) / 2.0

        y1 = pCorrV1.GetBinContent(bin)
        e1 = pCorrV1.GetBinError(bin)

        y2 = pCorrV2.GetBinContent(bin)
        e2 = pCorrV2.GetBinError(bin)

        if y2 == 0:
            ratio = 0
            ratio_error = 0
        else:
            ratio = y1 / y2
            ratio_error = math.sqrt((e1 / y2)**2 + (y1 * e2 / y2**2)**2)

        x_vals.append(x)
        y_vals.append(ratio)
        x_errs.append(0.0)
        y_errs.append(ratio_error)

    x_array = array.array('d', x_vals)
    y_array = array.array('d', y_vals)
    ex_array = array.array('d', x_errs)
    ey_array = array.array('d', y_errs)

    ratio_graph = ROOT.TGraphErrors(n_bins, x_array, y_array, ex_array, ey_array)
    ratio_graph.SetMarkerStyle(21)
    ratio_graph.SetMarkerSize(0.8)
    ratio_graph.SetLineColor(ROOT.kBlack)

    return ratio_graph

def main(root_file_path, output_file_path):
    if not os.path.isfile(root_file_path):
        print(f"Error: ROOT file '{root_file_path}' does not exist.")
        sys.exit(1)

    root_file = ROOT.TFile.Open(root_file_path, "READ")
    if not root_file or root_file.IsZombie():
        print(f"Error: Cannot open ROOT file '{root_file_path}'.")
        sys.exit(1)

    hist_pt_dir = root_file.Get("HistGivenEta")
    if not hist_pt_dir or not hist_pt_dir.IsFolder():
        print("Error: 'HistGivenEta' directory not found in the ROOT file.")
        sys.exit(1)

    pt_keys = hist_pt_dir.GetListOfKeys()
    n_pt_bins = pt_keys.GetEntries()
    print(f"Found {n_pt_bins} pt directories in 'HistGivenEta'.")

    cols, rows = get_grid_dimensions(n_pt_bins)
    print(f"Canvas will be divided into {cols} columns and {rows} rows.")

    pad_width = 600
    pad_height = 600
    canvas_width = rows * pad_width
    canvas_height = cols * pad_height
    canvas = ROOT.TCanvas("canvas_pCorr", "pCorrV1 vs pCorrV2 Across Eta Bins", canvas_width, canvas_height)

    canvas.Divide(rows, cols)
    ROOT.gStyle.SetOptStat(0)

    hist_list = []
    ratio_graphs = []
    line_list = []

    for i in range(n_pt_bins):
        key = pt_keys.At(i)
        pt_dir_name = key.GetName()
        print(f"Processing pt directory: {pt_dir_name}")

        pt_dir = hist_pt_dir.Get(pt_dir_name)
        if not pt_dir or not pt_dir.IsFolder():
            print(f"Warning: '{pt_dir_name}' is not a directory. Skipping.")
            continue

        tagName = "RunE_DATA_L2L3Residual_AK4PFPuppi"
        pCorrV1 = pt_dir.Get(f"pCorrV1_{tagName}")
        pCorrV2 = pt_dir.Get(f"pCorrV2_{tagName}")

        if not pCorrV1:
            print(f"Warning: 'pCorrV1' not found in '{pt_dir_name}'. Skipping.")
            continue
        if not pCorrV2:
            print(f"Warning: 'pCorrV2' not found in '{pt_dir_name}'. Skipping.")
            continue

        if not pCorrV1.InheritsFrom("TProfile"):
            print(f"Warning: 'pCorrV1' in '{pt_dir_name}' is not a TProfile. Skipping.")
            continue
        if not pCorrV2.InheritsFrom("TProfile"):
            print(f"Warning: 'pCorrV2' in '{pt_dir_name}' is not a TProfile. Skipping.")
            continue

        pad_number = i + 1
        canvas.cd(pad_number)

        # We now only create two pads: overlay and ratio
        # Let's allocate 70% height for overlay and 30% for ratio
        overlay_height = 0.7
        ratio_height = 0.3

        pad_overlay = ROOT.TPad(f"pad_overlay_{pad_number}", f"{pt_dir_name}_overlay", 0, ratio_height, 1, 1)
        pad_overlay.SetBottomMargin(0.02)
        pad_overlay.SetLeftMargin(0.12)
        pad_overlay.SetRightMargin(0.05)
        pad_overlay.SetTopMargin(0.20)
        pad_overlay.SetLogx(True);
        pad_overlay.Draw()
        pad_overlay.cd()

        # Clone histograms
        h1 = pCorrV1.Clone(f"h1_{i}")
        h2 = pCorrV2.Clone(f"h2_{i}")

        h1.SetTitle("")
        h2.SetTitle("")
        h1.GetYaxis().SetTitle("#splitline{Mean of Correction}{(%s)}"%tagName)
        h2.GetYaxis().SetTitle("#splitline{Mean of Correction}{(%s)}"%tagName)

        h1.GetXaxis().SetTitle("Jet #eta")
        h2.GetXaxis().SetTitle("Jet #eta")
        h1.SetLineColor(ROOT.kRed)
        h2.SetLineColor(ROOT.kBlue)
        h1.SetLineWidth(2)
        h2.SetLineWidth(2)
        h1.GetYaxis().SetTitleOffset(1.5)
        h2.GetYaxis().SetTitleOffset(1.5)

        # Draw histograms
        h1.Draw("E")
        h2.Draw("E SAME")

        # Store histograms
        hist_list.extend([h1, h2])

        # Add stats to the overlay as a title or via TLatex
        mean1 = pCorrV1.GetMean()
        error1 = pCorrV1.GetMeanError()
        entries1 = pCorrV1.GetEntries()

        mean2 = pCorrV2.GetMean()
        error2 = pCorrV2.GetMeanError()
        entries2 = pCorrV2.GetEntries()

        # Add TLatex to show stats in the overlay pad
        latexTit = ROOT.TLatex()
        latexTit.SetNDC()
        latexTit.SetTextFont(43)
        latexTit.SetTextSize(16)
        latexTit.SetTextColor(ROOT.kBlack)
        latexTit.DrawLatex(0.15, 0.95, pt_dir_name)

        statV1 = f"pCorrV1: Entries={int(entries1)}, Mean = {mean1:.3f}"
        latexV1 = ROOT.TLatex()
        latexV1.SetNDC()
        latexV1.SetTextFont(43)
        latexV1.SetTextSize(16)
        latexV1.SetTextColor(ROOT.kRed)
        latexV1.DrawLatex(0.15, 0.90, statV1)

        statV2 = f"pCorrV2:Entries={int(entries2)}, Mean = {mean2:.3f}"
        latexV2 = ROOT.TLatex()
        latexV2.SetNDC()
        latexV2.SetTextFont(43)
        latexV2.SetTextSize(16)
        latexV2.SetTextColor(ROOT.kBlue)
        latexV2.DrawLatex(0.15, 0.85, statV2)


        # Create ratio pad
        canvas.cd(pad_number)
        pad_ratio = ROOT.TPad(f"pad_ratio_{pad_number}", f"{pt_dir_name}_ratio", 0, 0.0, 1, ratio_height)
        pad_ratio.SetTopMargin(0.02)
        pad_ratio.SetLeftMargin(0.12)
        pad_ratio.SetRightMargin(0.05)
        pad_ratio.SetBottomMargin(0.35)
        pad_ratio.SetLogx(True);
        pad_ratio.Draw()
        pad_ratio.cd()

        # Compute ratio graph
        ratio_graph = compute_ratio_graph(pCorrV1, pCorrV2)
        ratio_graph.SetTitle("")
        ratio_graph.GetYaxis().SetTitle("pCorrV1 / pCorrV2")
        ratio_graph.GetYaxis().SetNdivisions(505)
        ratio_graph.GetYaxis().SetTitleSize(0.12)
        ratio_graph.GetYaxis().SetTitleFont(43)
        ratio_graph.GetYaxis().SetTitleOffset(1.0)
        ratio_graph.GetYaxis().SetLabelSize(0.1)
        ratio_graph.GetXaxis().SetTitleSize(0.12)
        ratio_graph.GetXaxis().SetTitleFont(43)
        ratio_graph.GetXaxis().SetTitleOffset(1.0)
        ratio_graph.GetXaxis().SetLabelSize(0.1)
        ratio_graph.SetMarkerStyle(21)
        ratio_graph.SetMarkerSize(0.8)
        ratio_graph.SetLineColor(ROOT.kBlack)
        ratio_graph.Draw("AP")
        ratio_graphs.append(ratio_graph)

        # Add horizontal line at y=1
        line = ROOT.TLine(ratio_graph.GetXaxis().GetXmin(), 1, ratio_graph.GetXaxis().GetXmax(), 1)
        line.SetLineColor(ROOT.kGray)
        line.SetLineStyle(2)
        line.Draw("same")
        line_list.append(line)

        canvas.cd()

    canvas.SaveAs(output_file_path)
    print(f"\nSaved all plots to {output_file_path}")

    root_file.Close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python plotGivenEta.py <input_root_file> <output_file>")
        sys.exit(1)

    input_root_file = sys.argv[1]
    output_file = sys.argv[2]

    main(input_root_file, output_file)

