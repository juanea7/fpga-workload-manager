# Workload Manager

This folder contains the source code and related files for the Workload Manager. The Workload Manager is designed to manage and execute various computational kernels, providing monitoring, scheduling and power/performance prediction functionalities.

## Related Components:

- [MachSuite](https://github.com/breagen/MachSuite): Provides the kernels used for the proposed use case.
- [ARTICo³](https://github.com/des-cei/artico3.git): Performs the hardware acceleration.
- [Monitor](https://github.com/juanea7/fpga-monitor.git): Enables the workload monitoring.
- [Modeling](https://github.com/juanea7/fpga-modeling.git): Enables the incremental run-time FPGA modeling.

## Folder Structure

- **data/**: Contains data files for the MachSuite kernels.
- **setup_monitor/**: Contains scripts and configurations for monitoring the setup.
- **src/**: Contains the source code for the Workload Manager.

## Usage

### Building the Application

To build the Workload Manager application, treat the code as an ARTICo³ application and build it with its design flow.

### Running the Application

After building the application, you can run it using the following command in the board:

```sh
./machsuite <number_of_workload_to_execute>
```

### Configuration

The Workload Manager can be configured using various configuration parameters located in the `src/application/debug.h` file. Each parameter enables a different Workload Manager component.