taskset -c 0,1,2,3 ./kmeans_omp -n 4 -i ../../data/kmeans/inpuGen/1000000_34.txt; pkill a.out
./kmeans_xitao_1 -n 4 -i ../../data/kmeans/inpuGen/1000000_34.txt; pkill a.out
