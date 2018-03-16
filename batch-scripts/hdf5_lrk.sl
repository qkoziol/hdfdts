#!/bin/bash -l
##SBATCH -p regular
##SBATCH -q shared
#SBATCH -p debug
#SBATCH -t 00:30:00
#SBATCH -N 1
##SBATCH -N 342 #171 #1366 #1366 #171 #342

#SBATCH -o hdf5_lrk
##SBATCH -o brtnfld.o%j
##SBATCH -C haswell   #Use Haswell nodes
#Edison has 24 cores per node

cd $SCRATCH
WRKDIR=$SCRATCH/hdf5_develop/build
#mkdir $WRKDIR
#lfs setstripe -c 64 -S 16m $WRKDIR
cd $WRKDIR
out="2"
cmd="./makecheck-s.sh"

#tsk=32768 # -N times 24
#tsk=4096
tsk=8193
#cleanup=yes

cp $SLURM_SUBMIT_DIR/$cmd .
ls
# find number of MPI tasks per node
#set TPN=`echo $SLURM_TASKS_PER_NODE | cut -f 1 -d \(`
# find number of CPU cores per node
#set PPN=`echo $SLURM_JOB_CPUS_PER_NODE | cut -f 1 -d \(`
#echo " $tsk $TPN $PPN $SLURM_TASKS_PER_NODE $SLURM_JOB_CPUS_PER_NODE"
#rm -f $SLURM_SUBMIT_DIR/${out}_$tsk
#for i in `seq 1 4`; do
#  srun -n $tsk ./$cmd ${out} 
#done
#export PMI_MMAP_SYNC_WAIT_TIME=300

srun -n 24 ./$cmd
ls -ho
#cp timing $SLURM_SUBMIT_DIR/timing_${tsk}_${out}

cd $SLURM_SUBMIT_DIR
#if [ -n "$cleanup" ]; then
#  rm -r -f $WRKDIR
#:fi
