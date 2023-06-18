for i in {1..8}; do
    cd "${i}threads"
    cat slurm-* > ./concat.txt
    rm slurm-*
    echo "Times for ${i} threads:" >> ../"${i}threads.txt"
    grep 'total_time_in_seconds' ./concat.txt | cut -d' ' -f2 >> ../"${i}threads.txt"
    echo "Throughput for ${i} threads:" >> ../"${i}threads.txt"
    grep 'Throughput' ./concat.txt | cut -d' ' -f2,3 >> ../"${i}threads.txt"
    cd ..
done
cat *threads.txt >> report.txt
rm *threads.txt
rm -rf *threads
