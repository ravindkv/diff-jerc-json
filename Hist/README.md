# Hist Package

## Table of Contents
- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
  - [1. Install CorrectionLib](#1-install-correctionlib)
  - [2. Set Environment Variables](#2-set-environment-variables)
- [Preparing Input Files](#preparing-input-files)
  - [1. Get List of Skim Files](#1-get-list-of-skim-files)
- [Compiling the Code](#compiling-the-code)
- [Running the Code Locally](#running-the-code-locally)
  - [1. Display Help Message](#1-display-help-message)
- [Submitting Condor Jobs](#submitting-condor-jobs)
  - [1. Create Condor Job Files](#1-create-condor-job-files)
  - [2. Submit Jobs](#2-submit-jobs)
  - [3. Monitor Condor Jobs](#3-monitor-condor-jobs)
- [Checking and Resubmitting Jobs](#checking-and-resubmitting-jobs)
  - [1. Check Finished Jobs](#1-check-finished-jobs)
  - [2. Resubmit Failed Jobs](#2-resubmit-failed-jobs)
  - [3. Repeat Checking Process](#3-repeat-checking-process)
- [Output Files](#output-files)
- [Additional Information](#additional-information)
- [Troubleshooting](#troubleshooting)
- [Contact](#contact)

## Introduction

The **Hist** package is designed to produce histogram files from skimmed NanoAOD trees. It utilizes a set of common classes and histogramming classes to process data and generate the desired histograms. The package supports both local execution and submission of jobs to a Condor batch system for efficient processing of large datasets.

## Prerequisites

Before you begin, ensure you have the following software installed:

- **Git**: To clone the repository.
- **Python 3**: Required for running Python scripts.
- **GCC**: For compiling the C++ code.
- **Make**: To build the project using the provided Makefile.
- **Condor**: If you plan to submit jobs to a Condor batch system.
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

### 1. Get List of Skim Files

The Hist package requires input files containing skimmed NanoAOD data. To generate the list of input files:

```bash
cd input
python3 getFiles.py
```

This script will produce files containing paths to the skimmed NanoAOD trees. Review the generated files to ensure they are correct.


## Compiling the Code

To compile the code, run:

```bash
cd ..
make -j4
```

This will compile all the necessary C++ files and create object files in the `obj` directory. The main executable `runMain` will be created in the root directory.

## Running the Code Locally

To produce histogram files from the skimmed data locally, use the `runMain` executable.

### 1. Display Help Message

To see the available options:

```bash
./runMain -h
```

This will display all the commands and options available for running the code.

## Submitting Condor Jobs

To process multiple files or large datasets, submit jobs to a Condor batch system.

### 1. Create Condor Job Files

Navigate to the `condor` directory and generate the job files:

```bash
cd condor
python createJobFiles.py
```

This script will generate job submission files and scripts in the `tmpSub` directory.

### 2. Submit Jobs

It's recommended to submit a few jobs first to ensure everything is working correctly. Then, submit all jobs:

```bash
cd tmpSub

# Submit all jobs
source submitAll.sh
```

### 3. Monitor Condor Jobs

Monitor the status of your Condor jobs using the following commands:

```bash
# Check the queue
condor_q

# Tail the output of a specific job
condor_tail <JobID>

# Analyze why jobs are not running
condor_q -better-analyze <JobID>
```

Replace `<JobID>` with the specific job ID from `condor_q`.

## Checking and Resubmitting Jobs

Once all jobs have completed, check for any failed jobs and resubmit them if necessary.

### 1. Check Finished Jobs

Navigate back to the `condor` directory and run the `checkFinishedJobs.py` script:

```bash
cd ..
python3 checkFinishedJobs.py
```

This script will:

- Open each output file.
- Perform checks to ensure the job ran successfully.
- Identify any failed jobs.
- Create JDL files for resubmitting failed jobs.

### 2. Resubmit Failed Jobs

If there are failed jobs, resubmit them:

```bash
cd tmpSub

# Submit the resubmission JDL file
condor_submit resubJobs.jdl
```

### 3. Repeat Checking Process

After the resubmitted jobs have completed, repeat the checking process:

```bash
cd ..
python3 checkFinishedJobs.py
```

## Output Files

The output root files are stored in the path specified in the `Inputs.py` file

## Additional Information

- **Common Classes**: The common classes such as `SkimTree.cpp`, `EventPick.cpp`, and `ObjectPick.cpp` have been updated. These are used by the histogramming classes (e.g., `HistZeeJet.cpp`, `HistGamJet.cpp`).
- **Main Executable**: All classes are included in `main.cpp`, which is compiled into the `runMain` executable.
- **Object Files**: Compiled object files are stored in the `obj` directory.
- **CorrectionLib Package**: The `corrlib` directory contains the CorrectionLib package required for corrections in the analysis.
- **Input Directory**: The `input` directory contains JSON paths of skimmed NanoAOD trees.
- **Output Directory**: The `output` directory stores the resulting histogram files.
- **Inputs.py**: The `Inputs.py` file controls flags such as the year or channel for Condor job submission.
- **POG and tmp Directories**: The `POG` directory may contain Physics Object Group related files, and the `tmp` directory is used for temporary files during processing.

## Contact

For any questions or issues, please contact [Ravindra Verma](mailto:rverma@cern.ch).

