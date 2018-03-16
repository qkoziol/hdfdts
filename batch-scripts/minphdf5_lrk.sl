#!/bin/bash -l
#SBATCH -p regular
#SBATCH -t 01:00:00
#SBATCH -N 1
#SBATCH -o hdf5_lrk
##SBATCH -C haswell   #Use Haswell nodes
#Edison has 24 cores per node
cd $SCRATCH
WRKDIR=$SCRATCH/hdf5_develop/parbuild
#mkdir $WRKDIR
#lfs setstripe -c 64 -S 16m $WRKDIR
cd $WRKDIR
out="2"
cmd="./makecheck-p.sh"
tsk=8193
cp $SLURM_SUBMIT_DIR/$cmd .
./$cmd
cd $SLURM_SUBMIT_DIR
