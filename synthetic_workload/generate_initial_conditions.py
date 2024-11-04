from numpy.random import default_rng
import numpy
import argparse
import struct
import os

# Initiate the parser
example_text = '''Example:

python generate_initial_conditions.py -p -n 20000 -t 250 --range-kernels 0 11 --base-number-executions 128 --range-executions 5 -s 42'''

parser = argparse.ArgumentParser(description="This script generates initial conditions for the setup", epilog=example_text, formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("-p", "--plot", help="<Requi>plot the density distribution of the generated initial conditions", action="store_true")
parser.add_argument("-n", "--number-kernels", metavar="X", help="set the number of kernels to generate", default=20000, type=int)
parser.add_argument("-t", "--time-arrival", metavar="X", help="set the time between kernels in ms", default=10, type=int)
parser.add_argument("--range-kernels", nargs=2, metavar=('start', 'end'), help="set the range of kernels to use (from start to end-1)", default=[0,11], type=int)
parser.add_argument("--base-number-executions", metavar="x", help="set the base number of executions per kernel", default=8, type=int)
parser.add_argument("--range-executions", metavar="x", help="set the range of the number of executions from the base", default=8, type=int)
parser.add_argument("-s","--seed", metavar="x", help="set the seed for the random generators", default=42, type=int)

# Read arguments from the command line
args = parser.parse_args()

if args.plot:
    import matplotlib.pyplot as plt

def kernel_inter_arrival_generator(ratio, num_elements, seed = 42):
    # Create random number generator with a seed
    rng = default_rng(seed)
    # Create exponential rng with specific parameters
    return rng.exponential(scale = 1/ratio, size = num_elements)  # scale == 1/lambda

def kernel_id_generator(id_range, num_elements, seed = 42):
    # Create random number generator with a seed
    rng = default_rng(seed)
    # Create exponential rng with specific parameters
    return rng.integers(low = id_range[0], high = id_range[1], size = num_elements)

def num_executions_generator(base, multiples, num_elements, seed = 42):
    # Create random number generator with a seed
    rng = default_rng(seed)
    # Create exponential rng with specific parameters
    list_values = rng.integers(low = 0, high = multiples, size = num_elements) # list of integers
    # Convert to powers of 2 starting from "base"
    return base * (2 ** list_values)

# Save randomly generated initial conditions in files
def save_generated_initial_conditions(inter_arrival_values, kernel_id_values, num_executions_values):

    # Create folder for setup generated initial conditions
    if not os.path.exists('initial_conditions'):
        os.makedirs('initial_conditions')

    file = open("initial_conditions/inter_arrival.bin", "wb")
    for value in inter_arrival_values:
        file.write(struct.pack('f', value))
    file.close()

    file = open("initial_conditions/kernel_id.bin", "wb")
    for value in kernel_id_values:
        file.write(struct.pack('i', value))
    file.close()

    file = open("initial_conditions/num_executions.bin", "wb")
    for value in num_executions_values:
        file.write(struct.pack('i', value))
    file.close()

    print("Initial conditions successfully saved into files.\n")

## Printing Functions ##

# Print kernels inter arrival dristributions
def print_kernerl_inter_arrival_data(list_values):

    inter_arrival_time = []
    arrival_time = []
    total_time = 0

    # Time printing
    for i, val in enumerate(list_values):
        inter_arrival_time.append(val)
        total_time += inter_arrival_time[i]
        arrival_time.append(total_time)

        #print("Inter-Arrival Time: {} | Arrival Time: {}".format(inter_arrival_time[i],round(arrival_time[i],3)))
    print("\n\nTotal Time: {} | Mean Inter-Arrival Time: {}".format(arrival_time[-1],sum(inter_arrival_time)/len(inter_arrival_time)))

    print("\n\nStandard Deviation: {} | Variance: {}\n\n".format(numpy.std(inter_arrival_time), numpy.var(inter_arrival_time)))

    # histogram
    count, bins, ignored = plt.hist(list_values, 200, density=True)

    plt.savefig('initial_conditions/inter_arrival_distribution.png')
    plt.close()

# Print kernel id distributions
def print_kernel_id_data(list_values, divs):
    print(list_values[0:15])
    # histogram
    count, bins, ignored = plt.hist(list_values, divs, density=True)

    plt.savefig('initial_conditions/kernel_id_distribution.png')
    plt.close()

# Print kernel number of executions dritributions
def print_num_executions_data(list_values,divs):
    print(list_values[0:15])
    # histogram
    count, bins, ignored = plt.hist(list_values, divs, density=True)

    plt.savefig('initial_conditions/num_executions_distribution.png')
    plt.close()


# Actual Code


num_elements = args.number_kernels
ratio_arrival = 1/args.time_arrival
range_kernels = args.range_kernels
base_num_executions = args.base_number_executions
range_executions = args.range_executions

print("\nNumber of kernels: {0}".format(num_elements))
print("Ratio of arrival (k/ms): {0}".format(ratio_arrival))
print("Range of kernels: {0}".format(range_kernels))
print("Base number of executions: {0}".format(base_num_executions))
print("Range of executions: {0}".format(range_executions))

inter_arrival_values = kernel_inter_arrival_generator(ratio_arrival, num_elements)
kernel_id_values = kernel_id_generator(range_kernels, num_elements)
num_executions_values = num_executions_generator(base_num_executions, range_executions, num_elements)

save_generated_initial_conditions(inter_arrival_values, kernel_id_values, num_executions_values)

if args.plot:
    print_kernerl_inter_arrival_data(inter_arrival_values)
    print_kernel_id_data(kernel_id_values,range_kernels[1])
    print_num_executions_data(num_executions_values,range_executions)
