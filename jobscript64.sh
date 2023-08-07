#!/bin/bash

#------------------------------------
#     SLURM SPECIFIC COMMANDS
#------------------------------------
# Gives a name for the job
#SBATCH --job-name=pc

# Ask the scheduler for Y CPU cores on the same compute node
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64

# Set the name of the output file
#SBATCH -o searching_OMP_2.%N.%j.out

# Set the name of the error file
#SBATCH -e searching_OMP_2.%N.%j.err

#------------------------------------------
#              OPENMP SPECIFIC
#------------------------------------------

# Prints date
date

# Compiling the Program
gcc -fopenmp -O0 searching_OMP_2.c -o searching_OMP_2 -lm

# Prints starting new job
echo "Starting new job"

# Executes the compiled program
./searching_OMP_1

# Prints finished job
echo "Finished job"

# Prints date
date

#------------------------------------------

