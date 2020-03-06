##############################################################################################
#  This program is part of the Barcelona OpenMP Tasks Suite                                  #
#  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  #
#  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   #
#                                                                                            #
#  This program is free software; you can redistribute it and/or modify                      #
#  it under the terms of the GNU General Public License as published by                      #
#  the Free Software Foundation; either version 2 of the License, or                         #
#  (at your option) any later version.                                                       #
#                                                                                            #
#  This program is distributed in the hope that it will be useful,                           #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of                            #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                             #
#  GNU General Public License for more details.                                              #
#                                                                                            #
#  You should have received a copy of the GNU General Public License                         #
#  along with this program; if not, write to the Free Software                               #
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA            #
##############################################################################################
include ../../makefile.sched

CFLAGS += -I.. -I. -O3 ${EXTRAEINC}
CXXFLAGS +=$(CFLAGS) -std=c++11 ${EXTRAEINC}

include makefile.heat.mpi
#CC=gcc
LIBS = -lm -L../../lib/ -lxitao -pthread ${EXTRAELIBS}
HEAT_TAO_OBJS=heat-tao.o misc.o
HEAT_TAO_MPI_OBJS=heat-tao-mpi.o misc.o
HEAT_TBB_OBJS=heat-tbb.o misc.o

LOIFLAGS :=



# options for LOI/KRD
ifdef LOI
LOI_SOURCES =  ../loi/loi.c ../loi/krd_common.c ../loi/krd_trace.c ../loi/krd_bench.c
LOI_OBJS    =  ../loi/loi.o ../loi/krd_common.o ../loi/krd_trace.o ../loi/krd_bench.o
LOI_DIR = ../loi
LOI_HDR = ../loi/loi.h ../loi/krd.h
else
LOI_DIR = ../loi
endif

CXXFLAGS += -I../../include
all: heat-tao-mpi

%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS)  $(LOIFLAGS) --std=gnu11 -c $< -o $@
 
%.o : %.cxx
	$(CXX) $(CXXFLAGS) $(CPPFLAGS)  $(LOIFLAGS) --std=c++11  -c $< -o $@
 
heat-tao: $(HEAT_TAO_OBJS) $(LOI_OBJS)
	$(CXX) $^ -o $@ ${LIBS}  

heat-tao-mpi: $(HEAT_TAO_MPI_OBJS) $(LOI_OBJS)
	$(CXX) $^ -o $@ ${LIBS}  

heat-tbb: $(HEAT_TBB_OBJS) misc.o
	$(CXX) $^ -o $@ ${LIBS} -ltbb

heat: heat.o solver.o misc.o
	$(CC) $^ -o $@ $(CFLAGS) ${LIBS}

clean:
	rm -rf $(HEAT_TAO_OBJS) $(HEAT_TBB_OBJS) $(HEAT_TAO_MPI_OBJS) heat-tao heat-tao-mpi heat-tbb *.ppm heat

run: 
	GOTAO_NTHREADS=2 mpirun -n 4 ./heat-tao-mpi small.dat
