import numpy as np
import matplotlib.pyplot as plt
import math
from matplotlib.backends.backend_pdf import PdfPages
import argparse

def build_function(expr, params):
    """
    Builds a function f(x) from a function expression string by substituting
    parameter placeholders [0] to [n] with the actual parameter values.
    
    Parameters:
      expr : str
          The function expression (e.g., "max(0.0001,((x<[10])*([9]))+((x>=[10])*([0]+...))")
      params : list of floats
          List of parameter values to substitute into the expression.
    
    Returns:
      f : function
          A function of x that evaluates the expression.
    """
    expr_instance = expr
    for i, p in enumerate(params):
        # Replace each placeholder [i] with the corresponding parameter value
        expr_instance = expr_instance.replace(f'[{i}]', str(p))
    # Define a function that evaluates the modified expression.
    def f(x):
        return eval(expr_instance, {"x": x, "log10": math.log10, "exp": math.exp, "pow": pow, "max": max})
    return f

def main(input_file, output_pdf, jet_pt_raw=56.7143):
    # Read all lines from the file.
    with open(input_file, 'r') as f:
        lines = f.readlines()
    
    # Find the header line (assumed to start with '{') and extract the function expression.
    header_expr = None
    for line in lines:
        line = line.strip()
        if not line:
            continue
        if line.startswith("{"):
            header_expr = line.strip("{}")
            break
    if header_expr is None:
        print("No header function found in the file.")
        return

    # Assume header format: ... JetPt <function_expression> Correction ...
    try:
        after_jetpt = header_expr.split("JetPt", 1)[1]
        func_expr = after_jetpt.split("Correction", 1)[0].strip()
    except Exception as e:
        print("Error parsing function expression from header:", e)
        return

    print("Function expression extracted from header:")
    print(func_expr)

    # Create a multipage PDF to save the plots.
    with PdfPages(output_pdf) as pdf:
        # Process each data line (skip header lines).
        for line in lines:
            line = line.strip()
            if not line:
                continue
            if line.startswith("{"):
                continue
            tokens = line.split()
            # Ensure there are enough tokens; here we expect at least 16 tokens per row.
            if len(tokens) < 16:
                continue

            # Extract eta bin info (first two tokens)
            eta_low = tokens[0]
            eta_high = tokens[1]

            try:
                # Extract the 11 parameters from tokens 6 to 16 (indices 5 to 15)
                params = [float(tok) for tok in tokens[5:16]]
            except Exception as e:
                print("Error converting parameters on line:", line)
                continue

            # Build the correction function using the header expression and these parameters.
            f = build_function(func_expr, params)
            
            # Generate a range of jet_pt values for plotting.
            jet_pts = np.linspace(10, 200, 500)
            try:
                corrections = [f(pt) for pt in jet_pts]
            except Exception as e:
                print("Error evaluating function for line:", line, "Error:", e)
                continue

            # Create the plot for this eta bin.
            plt.figure(figsize=(10, 6))
            plt.plot(jet_pts, corrections, label=f'Jet Eta [{eta_low}, {eta_high}]')
            plt.axvline(jet_pt_raw, color='red', linestyle='--', label=f'jet_pt_raw = {jet_pt_raw}')
            plt.xlabel('Jet Transverse Momentum (GeV)')
            plt.ylabel('Correction Factor')
            plt.title(f'JEC: Eta bin [{eta_low}, {eta_high}]')
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

