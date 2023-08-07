#!/bin/bash

#------------------------------------
#     SLURM SPECIFIC COMMANDS
#------------------------------------
# Gives a name for the job
#SBATCH --job-name=project_MPI

# Ask the scheduler to run N MPI processes on N compute nodes
#SBATCH --nodes=4
#SBATCH --ntasks=4

# Set the name of the output file
#SBATCH -o searchingMPI.%j.out

# Set the name of the error file
#SBATCH -e searchingMPI.%j.err

#------------------------------------------
#               MPI SPECIFIC
#------------------------------------------

# Load mpi module
module add mpi/openmpi

# Prints date
date
rm -f result_MPI.txt
touch result_MPI.txt
# Compiling the Program
bash ./execute_MPI large_inputs > output.txt
# Prints finished job
echo "Finished job"

# Prints date
date
#------------------------------------------


