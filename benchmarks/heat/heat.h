/*
 * heat.h
 *
 * Global definitions for the iterative solver
 */
#ifndef HEAT_H
#define HEAT_H
#pragma once
#if USE_MPI
#include <mpi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
    

#include <stdio.h>
  

// configuration

typedef struct
{
    float posx;
    float posy;
    float range;
    float temp;
}
heatsrc_t;

typedef struct
{
    unsigned maxiter;       // maximum number of iterations
    unsigned resolution;    // spatial resolution
    unsigned padding;       // added padding (border does not count).
                            // tipically block size - 1
    int algorithm;          // 0=>Jacobi, 1=>Gauss

    unsigned visres;        // visualization resolution
  
    double *u, *uhelp;
    double *uvis;
    double *ftmp;

    unsigned   numsrcs;     // number of heat sources
    heatsrc_t *heatsrcs;    
#if USE_MPI    
    MPI_Comm cartesian_topo_mpi_comm;    
#endif    
}
algoparam_t;

// function declarations

// misc.c
int initialize( algoparam_t *param, int no_padding );
int finalize( algoparam_t *param );
void write_image( FILE * f, double *u, unsigned padding,
		  unsigned sizex, unsigned sizey );
int coarsen(double *uold, unsigned oldx, unsigned oldy, unsigned padding,
	    double *unew, unsigned newx, unsigned newy );
int read_input( FILE *infile, algoparam_t *param );
void print_params( algoparam_t *param );
double wtime();

// solvers in solver.c
/*
double relax_redblack(unsigned sizey,  double (*u)[sizey], 
		  unsigned sizex );

double relax_gauss( unsigned padding, unsigned sizey, double (*u)[sizey+padding*2], 
		  unsigned sizex );

double relax_jacobi(unsigned sizey,  double (*u)[sizey], double (*utmp)[sizey],
		   unsigned sizex  ); 
*/
#ifdef __cplusplus
}
#endif
#endif