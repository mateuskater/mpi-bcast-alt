for i in {1..8}; do
    mkdir "${i}threads"
    cp broadcast ./"${i}threads"/broadcast
    cp myBcast-slurm.sh ./"${i}threads"/myBcast-slurm.sh
    cd "${i}threads"
    for j in {1..10}; do
        sbatch --exclusive -N "${i}" myBcast-slurm.sh
    done
    cd ..
done
