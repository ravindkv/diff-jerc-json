import ROOT
import os
import sys
import math
import json
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

def compute_ratio_graph(pCorrOld, pCorrNew):
    n_bins = pCorrOld.GetNbinsX()
    x_vals = []
    y_vals = []
    x_errs = []
    y_errs = []

    for bin in range(1, n_bins + 1):
        x = pCorrOld.GetBinCenter(bin)
        x_err = pCorrOld.GetBinWidth(bin) / 2.0

        y1 = pCorrOld.GetBinContent(bin)
        e1 = pCorrOld.GetBinError(bin)

        y2 = pCorrNew.GetBinContent(bin)
        e2 = pCorrNew.GetBinError(bin)

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

def plot_for_tag(root_file, tagName, statV1, statV2, output_file_path, HistGivenVar):
    print(f"\nOld: {statV1}, New: {statV2}\n")
    hist_var_dir = root_file.Get(HistGivenVar)
    if not hist_var_dir or not hist_var_dir.IsFolder():
        print(f"Error: {HistGivenVar} directory not found in the ROOT file.")
        sys.exit(1)

    var_keys = hist_var_dir.GetListOfKeys()
    n_var_bins = var_keys.GetEntries()
    print(f"Found {n_var_bins} pt directories in {HistGivenVar}.")

    cols, rows = get_grid_dimensions(n_var_bins)
    print(f"Canvas will be divided into {cols} columns and {rows} rows.")

    pad_width = 600
    pad_height = 600
    canvas_width = rows * pad_width
    canvas_height = cols * pad_height
    canvas = ROOT.TCanvas(f"canvas_pCorr_{tagName}_{HistGivenVar}", f"pCorrOld vs pCorrNew: {tagName}", canvas_width, canvas_height)


    canvas.Divide(rows, cols)
    ROOT.gStyle.SetOptStat(0)

    hist_list = []
    ratio_graphs = []
    line_list = []

    for i in range(n_var_bins):
        key = var_keys.At(i)
        var_dir_name = key.GetName()
        print(f"Processing pt directory: {var_dir_name}")

        var_dir = hist_var_dir.Get(var_dir_name)
        if not var_dir or not var_dir.IsFolder():
            print(f"Warning: '{var_dir_name}' is not a directory. Skipping.")
            continue

        pCorrOld = var_dir.Get(f"pCorrOld_{tagName}")
        pCorrNew = var_dir.Get(f"pCorrNew_{tagName}")

        if not pCorrOld:
            print(f"Warning: 'pCorrOld' not found in '{var_dir_name}'. Skipping.")
            continue
        if not pCorrNew:
            print(f"Warning: 'pCorrNew' not found in '{var_dir_name}'. Skipping.")
            continue

        if not pCorrOld.InheritsFrom("TProfile"):
            print(f"Warning: 'pCorrOld' in '{var_dir_name}' is not a TProfile. Skipping.")
            continue
        if not pCorrNew.InheritsFrom("TProfile"):
            print(f"Warning: 'pCorrNew' in '{var_dir_name}' is not a TProfile. Skipping.")
            continue

        pad_number = i + 1
        canvas.cd(pad_number)

        # We now only create two pads: overlay and ratio
        # Let's allocate 70% height for overlay and 30% for ratio
        overlay_height = 0.7
        ratio_height = 0.3

        pad_overlay = ROOT.TPad(f"pad_overlay_{pad_number}", f"{var_dir_name}_overlay", 0, ratio_height, 1, 1)
        if "Eta" in HistGivenVar: 
            pad_overlay.SetLogx(1)
        pad_overlay.SetBottomMargin(0.02)
        pad_overlay.SetLeftMargin(0.12)
        pad_overlay.SetRightMargin(0.05)
        pad_overlay.SetTopMargin(0.20)
        pad_overlay.Draw()
        pad_overlay.cd()

        # Clone histograms
        h1 = pCorrOld.Clone(f"h1_{i}")
        h2 = pCorrNew.Clone(f"h2_{i}")

        h1.SetTitle("")
        h2.SetTitle("")
        h1.GetYaxis().SetTitle("Mean of Correction")

        h1.GetXaxis().SetTitle("Jet #eta")
        h2.GetXaxis().SetTitle("Jet #eta")
        h1.SetLineColor(ROOT.kRed)
        h2.SetLineColor(ROOT.kBlue)
        h1.SetLineWidth(2)
        h2.SetLineWidth(2)
        h1.GetYaxis().SetTitleOffset(1.5)
        h2.GetYaxis().SetTitleOffset(1.5)

        # Draw histograms
        h1.Draw("EP")
        h2.Draw("EP SAME")

        # Store histograms
        hist_list.extend([h1, h2])

        # Add stats to the overlay as a title or via TLatex
        mean1 = pCorrOld.GetMean()
        error1 = pCorrOld.GetMeanError()
        entries1 = pCorrOld.GetEntries()

        mean2 = pCorrNew.GetMean()
        error2 = pCorrNew.GetMeanError()
        entries2 = pCorrNew.GetEntries()

        # Add TLatex to show stats in the overlay pad
        latexTit = ROOT.TLatex()
        latexTit.SetNDC()
        latexTit.SetTextFont(43)
        latexTit.SetTextSize(16)
        latexTit.SetTextColor(ROOT.kBlack)
        latexTit.DrawLatex(0.15, 0.95, var_dir_name)

        latexV1 = ROOT.TLatex()
        latexV1.SetNDC()
        latexV1.SetTextFont(43)
        latexV1.SetTextSize(16)
        latexV1.SetTextColor(ROOT.kRed)
        latexV1.DrawLatex(0.15, 0.90, statV1)

        latexV2 = ROOT.TLatex()
        latexV2.SetNDC()
        latexV2.SetTextFont(43)
        latexV2.SetTextSize(16)
        latexV2.SetTextColor(ROOT.kBlue)
        latexV2.DrawLatex(0.15, 0.85, statV2)


        # Create ratio pad
        canvas.cd(pad_number)
        pad_ratio = ROOT.TPad(f"pad_ratio_{pad_number}", f"{var_dir_name}_ratio", 0, 0.0, 1, ratio_height)
        if "Eta" in HistGivenVar: 
            pad_ratio.SetLogx(1)
        pad_ratio.SetTopMargin(0.02)
        pad_ratio.SetLeftMargin(0.12)
        pad_ratio.SetRightMargin(0.05)
        pad_ratio.SetBottomMargin(0.35)
        pad_ratio.Draw()
        pad_ratio.cd()

        # Compute ratio graph
        ratio_graph = compute_ratio_graph(pCorrOld, pCorrNew)
        ROOT.SetOwnership(ratio_graph, False)
        # force the ratio axis to match the overlay axis
        xmin = pCorrOld.GetXaxis().GetXmin()
        xmax = pCorrOld.GetXaxis().GetXmax()
        ratio_graph.GetXaxis().SetLimits(xmin, xmax)        # for TGraphAsymmErrors
        ratio_graph.GetXaxis().SetRangeUser(xmin, xmax)      # just in case
        ratio_graph.SetTitle("")
        ratio_graph.GetYaxis().SetTitle("pCorrOld / pCorrNew")
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
    return canvas


def main():
    if len(sys.argv) != 4:
        print("Usage: python plotGivenPt.py <input_root_file> <output_file> <metadata_json>")
        sys.exit(1)

    input_root = sys.argv[1]
    output_file = sys.argv[2]
    metadata_file = sys.argv[3]

    if not os.path.isfile(input_root):
        print(f"ERROR: {input_root} not found.")
        sys.exit(1)
    if not os.path.isfile(metadata_file):
        print(f"ERROR: metadata file {metadata_file} not found.")
        sys.exit(1)


    root_file = ROOT.TFile.Open(input_root, "READ")
    if not root_file or root_file.IsZombie():
        print(f"Error: Cannot open ROOT file '{root_file_path}'.")
        sys.exit(1)

    try:
        with open(metadata_file, 'r') as jf:
            meta = json.load(jf)
    except Exception as e:
        print(f"ERROR: failed to read metadata JSON: {e}")
        sys.exit(1)

    HistGivenVars = ["HistGivenPt", "HistGivenEta"]
    # build one canvas per tag
    canvases = []
    for HistGivenVar in HistGivenVars:
        for tag, entries in meta.items():
            if len(entries) < 2:
                continue
            statV1, statV2 = entries[0][1], entries[1][1]
            c = plot_for_tag(root_file, tag, statV1, statV2, output_file, HistGivenVar)
            canvases.append(c)

    # now write them into a single multi‐page PDF
    out = output_file  # e.g. "tmp.pdf"
    for i, c in enumerate(canvases):
        if i == 0:
            c.Print(f"{out}(")   # start document
        elif i == len(canvases) - 1:
            c.Print(f"{out})")  # end document
        else:
            c.Print(out)        # middle pages
        c.Close()

    root_file.Close()

if __name__ == "__main__":
    main()

