# Remote Execution Scripts

This folder contains scripts for setting up and executing remote workloads on a remote board. It also enables trace acquisition from remote boards.

## Folder Structure

- **local_setup_execution.sh**: Bash script localy in the remote board. It is managed by the `remote_setup_execution.sh`script.
- **receive_traces_over_socket_server_tcp.py**: Python script to receive trace data over a TCP socket server from a remote board.
- **remote_setup_execution.sh**: Bash script to orchestrate multiple sequential executions of the setup on the remote board.

## Installation

Before running any scripts, ensure you have the necessary permissions and SSH access to the remote server.

## Usage

### Remote Traces Acquisition

To receive traces over a TCP socket from the Workload Manager executed in a remote board, run the `receive_traces_over_socekt_server_tcp.py`script:

```bash
python receive_traces_over_socekt_server_tcp.py <path_to_store_the_traces>
```

_Note: The directory chosen to store the traces must already exists._

### Remote Setup Execution

To execute the setup on the remote board, run the `remote_setup_execution.sh` script:

```bash
./remote_setup_execution.sh
```

### Configuration

The `remote_setup_execution.sh` script contains configuration parameters such as remote server details, paths to executables, and directories. Ensure these parameters are correctly set before running the script. The current version is ad-hoc to our setup.