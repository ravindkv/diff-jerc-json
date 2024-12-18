# Hist Package


## Introduction

The **Hist** package is designed to produce histogram files from NanoAOD trees. It utilizes a set of common classes and histogramming classes to process data and generate the desired histograms. 

## Prerequisites

Before you begin, ensure you have the following software installed:

- **Git**: To clone the repository.
- **Python 3**: Required for running Python scripts.
- **GCC**: For compiling the C++ code.
- **Make**: To build the project using the provided Makefile.
- **CorrectionLib**: For applying corrections in the analysis.
- **ROOT**: For handling ROOT files and histograms.

## Installation

### 1. Install CorrectionLib

The Hist package depends on the **correctionlib** library. Follow these steps to install it:

```bash
cd Hist

# Clone the correctionlib repository into the 'corrlib' directory
git clone --recursive git@github.com:cms-nanoAOD/correctionlib.git corrlib

# Navigate to the corrlib directory
cd corrlib

# Build and install correctionlib
make
make install

# Return to the Hist package root directory
cd ..
```

### 2. Set Environment Variables

After installing CorrectionLib, update your `LD_LIBRARY_PATH` to include the CorrectionLib library:

```bash
export LD_LIBRARY_PATH=$(pwd)/corrlib/lib:$LD_LIBRARY_PATH
```

To make this change permanent, add the above line to your `~/.bashrc` file:

```bash
echo 'export LD_LIBRARY_PATH=$(pwd)/corrlib/lib:$LD_LIBRARY_PATH' >> ~/.bashrc

# Reload your bash configuration
source ~/.bashrc
```

## Preparing Input Files

### 1. Get List of NanoAOD Files

```bash
cd input/root
python3 getFiles.py
```

### 2. Get the two JERC json to be compared

```bash
cd ../..
cd input/jerc
```
Copy the two jsons in this directory, then

```bash
gunzip jet_jerc_V1.json.gz
gunzip jet_jerc_V2.json.gz
python createMetaJson.py --v1 jet_jerc_V1.json --v2 jet_jerc_V2.json 
```
This will create metadata.json file. Have a look at that.

## Compiling the Code

To compile the code, run:

```bash
cd ../..
make -j4
```

This will compile all the necessary C++ files and create object files in the `obj` directory. The main executable `runMain` will be created in the root directory.

## Running the Code Locally

To see the available options:

```bash
./runMain -h
```

This will display all the commands and options available for running the code. Run any of the command.

## Output Files

The output root files are stored in the output directory. 


## Plot the histograms


```bash
python plotDiff.py --root_file output/Data_ZeeJet_2024C_EGamma1_Hist_1of1000.root --metadata input/jerc/metadata.json 
```


## Additional Information

- **Main Executable**: All classes are included in `main.cpp`, which is compiled into the `runMain` executable.
- **Object Files**: Compiled object files are stored in the `obj` directory.
- **CorrectionLib Package**: The `corrlib` directory contains the CorrectionLib package required for corrections in the analysis.
- **Input Directory**: The `input` directory contains JSON paths of skimmed NanoAOD trees.
- **Output Directory**: The `output` directory stores the resulting histogram files.

## Contact

For any questions or issues, please contact [Ravindra Verma](mailto:rverma@cern.ch).

