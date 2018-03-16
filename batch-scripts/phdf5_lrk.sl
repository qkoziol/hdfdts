#!/bin/bash -l
#SBATCH -p regular
##SBATCH -p debug
##SBATCH -q shared
#SBATCH -t 02:00:00

#SBATCH -N 6
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
./$cmd
#cp timing $SLURM_SUBMIT_DIR/timing_${tsk}_${out}
cd $SLURM_SUBMIT_DIR
#if [ -n "$cleanup" ]; then
#  rm -r -f $WRKDIR
#:fi
