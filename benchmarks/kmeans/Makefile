# C compiler
CC = g++
CC_FLAGS = -fopenmp -O3 
include ../../makefile.sched
CC_FLAGS+=$(CXXFLAGS) -I../../include 
kmeans: cluster.o getopt.o kmeans.o kmeans_clustering.o 
	$(CC) $(CC_FLAGS) cluster.o getopt.o kmeans.o kmeans_clustering.o -L../../lib -lxitao  -o kmeans

kmeans_xitao: cluster.o getopt.o kmeans.o kmeans_clustering_xitao.o 
	$(CC) $(CC_FLAGS) cluster.o getopt.o kmeans.o kmeans_clustering.o -L../../lib -lxitao  -o kmeans_xitao

%.o: %.[ch]
	$(CC) $(CC_FLAGS) -c $< -o $@

cluster.o: cluster.c 
	$(CC) $(CC_FLAGS) cluster.c -c
	
getopt.o: getopt.c 
	$(CC) $(CC_FLAGS) getopt.c -c
	
kmeans.o: kmeans.c 
	$(CC) $(CC_FLAGS) kmeans.c -c

kmeans_clustering.o: kmeans_clustering.c kmeans.h
	$(CC) $(CC_FLAGS) kmeans_clustering.c -c

kmeans_clustering_xitao.o: kmeans_clustering.c kmeans.h
	$(CC) $(CC_FLAGS) -DUSE_XITAO kmeans_clustering.c -c

run: kmeans
	./kmeans -n 4 -i ./data/kmeans/kdd_cup 

run_xitao: kmeans_xitao
	./kmeans_xitao -n 4 -i ./data/kmeans/kdd_cup 

clean:
	rm -f *.o *~ kmeans kmeans_xitao 
