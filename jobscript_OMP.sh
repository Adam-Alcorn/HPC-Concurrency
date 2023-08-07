#!/bin/bash
#------------------------------------
#     SLURM SPECIFIC COMMANDS
#------------------------------------
# Gives a name for the job
#SBATCH --job-name=project_OMP

# Ask the scheduler to run N MPI processes on N compute nodes
#SBATCH --nodes=4
#SBATCH --ntasks=4

# Set the name of the output file
#SBATCH -o searchingOMP.%j.out

# Set the name of the error file
#SBATCH -e searchingOMP.%j.err

#------------------------------------------
#               OMP SPECIFIC
#------------------------------------------
rm -f result_OMP.txt

./execute_OMP large_inputs > output.txt
