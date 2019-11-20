export OMP_NUM_THREADS=2  
taskset -c 0,1 /home/musabdu/Repos/xitao/benchmarks/kmeans/interference/a.out 5000 &
