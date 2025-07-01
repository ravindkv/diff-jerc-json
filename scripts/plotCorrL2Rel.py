import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import argparse
import textwrap
import re

def build_function(expr, params):
    """
    Builds a vectorized function f(x) from a function expression string by substituting
    parameter placeholders [0] to [n] with the corresponding parameter values.
    """
    expr_instance = expr
    for i, p in enumerate(params):
        expr_instance = expr_instance.replace(f'[{i}]', str(p))
    
    # Define a vectorized "max" function that applies np.maximum elementwise.
    def max_np(*args):
        result = args[0]
        for arg in args[1:]:
            result = np.maximum(result, arg)
        return result

    # Build a vectorized function using numpy math functions.
    def f(x):
        # x will be a NumPy array.
        return eval(expr_instance, {
            "x": x,
            "log10": np.log10,
            "exp": np.exp,
            "pow": np.power,
            "max": max_np,
            "np": np
        })
    return f

def main(input_file, output_pdf, jet_pt_raw=56.7143):
    # Read all lines from the input file.
    with open(input_file, 'r') as f:
        lines = f.readlines()
    
    # Extract header (assumed to be the first line starting with '{')
    header_expr = None
    for line in lines:
        line = line.strip()
        if line and line.startswith("{"):
            header_expr = line.strip("{}")
            break
    if header_expr is None:
        print("No header function found in the file.")
        return

    # Extract the function expression assuming header format has "JetPt" and "Correction"
    try:
        after_jetpt = header_expr.split("JetPt", 1)[1]
        func_expr = after_jetpt.split("Correction", 1)[0].strip()
    except Exception as e:
        print("Error parsing function expression from header:", e)
        return

    print("Function expression extracted from header:")
    print(func_expr)

    # Create a multipage PDF for output plots.
    with PdfPages(output_pdf) as pdf:
        # Process each non-header line.
        for line in lines:
            line = line.strip()
            if not line or line.startswith("{"):
                continue

            tokens = line.split()
            allRows = len(tokens)
            moreRows = 0
            if allRows > 16:
                moreRows = 2

            print(allRows, moreRows)
            if len(tokens) < 14+moreRows:
                continue

            # Extract eta bin info (first two tokens)
            eta_low = tokens[0]
            eta_high = tokens[1]

            # Extract pT validity range from tokens[5] and tokens[6]
            try:
                pt_min = float(tokens[3+moreRows])
                pt_max = float(tokens[4+moreRows])
            except Exception as e:
                print("Error converting pT validity range on line:", line)
                continue

            try:
                # Extract 11 parameters (assuming they are in tokens 7 to 17)
                params = [float(tok) for tok in tokens[5+moreRows:16+moreRows]]
            except Exception as e:
                print("Error converting parameters on line:", line)
                continue

            # Build the correction function using the header expression and parameters.
            f = build_function(func_expr, params)
            
            # Generate jet_pt values.
            jet_pts = np.linspace(5, 2000, 50000)
            try:
                # Evaluate the vectorized function on all jet_pts at once.
                corrections = f(jet_pts)
            except Exception as e:
                print("Error evaluating function for line:", line, "Error:", e)
                continue
            
            # --- NEW: Check for problematic correction values in the pT validity range ---
            # Create a mask for jet_pts in the [pt_min, pt_max] range.
            valid_mask = (jet_pts >= pt_min) & (jet_pts <= pt_max)
            corrections_valid = corrections[valid_mask]
            jet_pts_valid = jet_pts[valid_mask]

            # Check if any correction in the region is below 0.0002 or above 5.
            if np.any(corrections_valid < 0.0002) or np.any(corrections_valid > 5):
                print(f"Warning: For eta bin [{eta_low}, {eta_high}], corrections within pT range {pt_min} -- {pt_max} have out-of-bound values:")
                print(f"   min correction: {np.min(corrections_valid)}")
                print(f"   max correction: {np.max(corrections_valid)}")

                # Find indices where corrections are below the lower threshold or above the upper threshold.
                out_low_indices = np.where(corrections_valid < 0.0002)[0]
                out_high_indices = np.where(corrections_valid > 5)[0]

                # Print the pT values (and corresponding correction values) for corrections below the threshold.
                if out_low_indices.size > 0:
                    print("   Corrections below 0.0002 found at:")
                    for idx in out_low_indices:
                        print(f"      pT = {jet_pts_valid[idx]:.3f} GeV  -->  correction = {corrections_valid[idx]:.3e}")

                # Print the pT values (and corresponding correction values) for corrections above the threshold.
                if out_high_indices.size > 0:
                    print("   Corrections above 5 found at:")
                    for idx in out_high_indices:
                        print(f"      pT = {jet_pts_valid[idx]:.3f} GeV  -->  correction = {corrections_valid[idx]:.3e}")


            # Create the plot for the current eta bin.
            plt.figure(figsize=(10, 6))
            plt.plot(jet_pts, corrections, label=f'Jet Eta [{eta_low}, {eta_high}]')
            
            # Draw vertical dashed lines at the pT validity range edges.
            plt.axvline(x=pt_min, color='green', linestyle='--', label=f'valid pT min = {pt_min}')
            plt.axvline(x=pt_max, color='green', linestyle='--', label=f'valid pT max = {pt_max}')

            plt.xlabel('x = Jet pT (GeV)')
            plt.xscale('log')
            plt.ylabel(f'Correction : {input_file}')

            wrapped_expr = textwrap.fill(func_expr, width=60)  # Adjust width as needed
            plt.title(f'{wrapped_expr}')

            # Prepare the parameter info string, e.g. "[0] = value0\n[1] = value1\n..."
            param_text = "\n".join([f"[{i}] = {p}" for i, p in enumerate(params)])
            # Display the parameter info in the top-left of the plot.
            plt.text(0.75, 0.80, param_text, transform=plt.gca().transAxes,
                     fontsize=10, verticalalignment='top',
                     bbox=dict(facecolor='white', alpha=0.5))

            plt.legend()
            plt.grid(True)
            pdf.savefig()
            plt.close()
    print(f"Plots saved to {output_pdf}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Plot JEC correction functions from a .txt file with a dynamic function expression."
    )
    parser.add_argument("input_file", type=str, help="Input .txt file containing JEC parameters and function expression")
    parser.add_argument("output_pdf", type=str, help="Output PDF file to save the plots")

    args = parser.parse_args()
    main(args.input_file, args.output_pdf)

