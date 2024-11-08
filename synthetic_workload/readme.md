# Synthetic Workload Manager

This folder contains scripts and data for generating synthetic workloads for the application.

## Folder Structure

- **pre-generated_data**: Directories containing pre-generated synthetic workloads for different scenarios.
- **generate_initial_conditions.py**: Python script to generate initial conditions for the synthetic workloads.
- **generation_example.txt**: Example of a workload generation command.
- **requirements.txt**: A file listing the Python dependencies required to run the scripts.

## Installation

Before running any scripts, install the necessary dependencies using the `requirements.txt` file:

```bash
pip install -r requirements.txt
```

## Usage

### Generating Initial Conditions

To generate initial conditions for the synthetic workloads, run the `generate_initial_conditions.py` script:

```bash
python generate_initial_conditions.py
```

### Configuration

The `generation_example.txt` file contains an example of generating a synthetic workload using the `generate_initial_conditions.py` script. The generated synthetic workload generated with the script will vary based on its configuration parameters, as shown in this file.

### Pre-generated Data

The `pre-generated_data`directory contains multiple already generated synthetic workloads for different scenarios. Use these directories as needed for your workload generation.