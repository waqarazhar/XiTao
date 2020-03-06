// solver.h -- jacobi solver as a TAO_PAR_FOR_2D_BASE class
#include "heat.h"
#include "tao.h"
#include "tao_parfor2D.h"
#include<assert.h>
#ifdef DO_LOI
#include "loi.h"
#endif

// chunk sizes for KRD
#define KRDBLOCKX 128  // 128KB
#define KRDBLOCKY 128

#define JACOBI2D 0
#define COPY2D 1

#define min(a,b) ( ((a) < (b)) ? (a) : (b) )

#if USE_MPI
class boundary_comm_tao : public AssemblyTask{
  MPI_Comm& mpi_cartesian_comm;  
  int mpi_rank;
  int rows;
  int cols;
  double* data;
  mpi_boundary_pair* source_dist_pairs;
public:  
  double** boundary_recv_data;
  double** boundary_send_data;
  boundary_comm_tao (MPI_Comm& _mpi_cartesian_comm,
          int _mpi_rank,
          int _rows,   // by convention: x axis
          int _cols,   // by convention: y axis
          double* _data,
          mpi_boundary_pair* _source_dist_pairs
          ) : mpi_cartesian_comm (_mpi_cartesian_comm), 
              mpi_rank(_mpi_rank),
              rows(_rows),
              cols(_cols),
              data(_data),
              source_dist_pairs(_source_dist_pairs),
              AssemblyTask(2)
              {            
                criticality = 1;
                boundary_recv_data = new double* [4];     
                boundary_send_data = new double* [4];
                
                boundary_recv_data[ste_direction::RIGHT] = new double[rows];
                memset(boundary_recv_data[ste_direction::RIGHT], 0.0, rows*sizeof(double)); 
                
                boundary_recv_data[ste_direction::LEFT] = new double[rows];
                memset(boundary_recv_data[ste_direction::LEFT], 0.0, rows*sizeof(double)); 
                
                boundary_send_data[ste_direction::RIGHT] = new double[rows];
                memset(boundary_send_data[ste_direction::RIGHT], 0.0, rows*sizeof(double)); 
                
                boundary_send_data[ste_direction::LEFT] = new double[rows];
                memset(boundary_send_data[ste_direction::LEFT], 0.0, rows*sizeof(double)); 

                boundary_recv_data[ste_direction::UP] = new double[cols];
                memset(boundary_recv_data[ste_direction::UP], 0.0, cols*sizeof(double));                 

                boundary_recv_data[ste_direction::DOWN] = new double[cols];                
                memset(boundary_recv_data[ste_direction::DOWN], 0.0, cols*sizeof(double));                 

                boundary_send_data[ste_direction::UP] = new double[cols];
                memset(boundary_send_data[ste_direction::UP], 0.0, cols*sizeof(double));                 
                
                boundary_send_data[ste_direction::DOWN] = new double[cols];
                memset(boundary_send_data[ste_direction::DOWN], 0.0, cols*sizeof(double));                 
              }

  void execute(int threadid) {      
    int tid = threadid - leader;
    if(tid == 0) {
        MPI_Request request[8];                
        MPI_Status status[8];   
        int req_index = 0;
        int curr_index = 0;
        MPI_Sendrecv(data, cols, MPI_DOUBLE,
                source_dist_pairs[ste_direction::UP].destination, 1,
                boundary_recv_data[ste_direction::UP], cols, MPI_DOUBLE,
                source_dist_pairs[ste_direction::UP].destination, 1,
                mpi_cartesian_comm, MPI_STATUS_IGNORE);

        MPI_Sendrecv(data + ((rows - 1) * cols), cols, MPI_DOUBLE,
                source_dist_pairs[ste_direction::DOWN].destination, 1,
                boundary_recv_data[ste_direction::DOWN], cols, MPI_DOUBLE,
                source_dist_pairs[ste_direction::DOWN].destination, 1,
                mpi_cartesian_comm, MPI_STATUS_IGNORE);

        for(int i = 0; i < rows; ++i) {
          boundary_send_data[ste_direction::RIGHT][i] = data[cols * i + cols - 1];        
        }

        MPI_Sendrecv(boundary_send_data[ste_direction::RIGHT], rows, MPI_DOUBLE,
                source_dist_pairs[ste_direction::RIGHT].destination, 0,
                boundary_recv_data[ste_direction::RIGHT], rows, MPI_DOUBLE,
                source_dist_pairs[ste_direction::RIGHT].destination, 0,
                mpi_cartesian_comm, MPI_STATUS_IGNORE);
        
        for(int i = 0; i < rows; ++i) {
          boundary_send_data[ste_direction::LEFT][i] = data[cols * i];        
        }
        
        MPI_Sendrecv(boundary_send_data[ste_direction::LEFT], rows, MPI_DOUBLE,
                source_dist_pairs[ste_direction::LEFT].destination, 0,
                boundary_recv_data[ste_direction::LEFT], rows, MPI_DOUBLE,
                source_dist_pairs[ste_direction::LEFT].destination, 0,
                mpi_cartesian_comm, MPI_STATUS_IGNORE);
      }
    }
  

  void cleanup() {
    // for(int i = 0; i < 4; ++i) {
    //   delete[] boundary_recv_data[i];
    //   delete[] boundary_send_data[i];
    // }
    // delete[] boundary_recv_data;
    // delete[] boundary_send_data;
  }
};


class jacobi2D : public TAO_PAR_FOR_2D_BASE
{
    public:
#if defined(CRIT_PERF_SCHED)
  static float time_table[][XITAO_MAXTHREADS];    
#endif
      double** boundary_data;
      jacobi2D(void *a, void*c, int rows, int cols, int offx, int offy, int chunkx, int chunky, 
                         gotao_schedule_2D sched, int ichunkx, int ichunky, int width, double** _boundary_data ,
                         float sta=GOTAO_NO_AFFINITY,                                 
                         int nthread=0) 
                         : TAO_PAR_FOR_2D_BASE(a,c,rows,cols,offx,offy,chunkx,chunky,
                                         sched,ichunkx,ichunky,width,sta), boundary_data(_boundary_data) {                          
                         }

                int ndx(int a, int b){ return a*gotao_parfor2D_cols + b; }

                int compute_for2D(int offx, int offy, int chunkx, int chunky)
                {                    
                    double *in = (double *) gotao_parfor2D_in;
                    double *out = (double *) gotao_parfor2D_out;
                    double diff; double sum=0.0;

                    // global rows and cols
                    int grows = gotao_parfor2D_rows;
                    int gcols = gotao_parfor2D_cols;          
                    int xstop = ((offx + chunkx) >= grows)? grows: offx + chunkx;
                    int ystop = ((offy + chunky) >= gcols)? gcols: offy + chunky;
                    int xstart = offx;
                    int ystart = offy;
#if DO_LOI
    kernel_profile_start();
#endif
                    for (int i=xstart; i<xstop; i++) 
                        for (int j=ystart; j<ystop; j++) {
                        double left   = (j - 1) >= 0? in[ndx(i,j-1)]: boundary_data[ste_direction::LEFT][i];
                        double right  = (j + 1) < gcols? in[ndx(i,j+1)]:boundary_data[ste_direction::RIGHT][i];
                        double top    = (i - 1) >= 0? in[ndx(i-1,j)] : boundary_data[ste_direction::UP][j];                        
                        double bottom = (i + 1) < grows? in[ndx(i+1,j)] : boundary_data[ste_direction::DOWN][j];
                        out[ndx(i,j)]= 0.25 * (left +  // left
                                               right +  // right
                                               top +  // top
                                               bottom); // bottom
                        }                    

		return 0;
                }

#if defined(CRIT_PERF_SCHED)
  void set_timetable(int threadid, float ticks, int index){
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index){ 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif
};
#else 
class jacobi2D : public TAO_PAR_FOR_2D_BASE
{
    public:
#if defined(CRIT_PERF_SCHED)
  static float time_table[][XITAO_MAXTHREADS];
#endif
      jacobi2D(void *a, void*c, int rows, int cols, int offx, int offy, int chunkx, int chunky, 
                         gotao_schedule_2D sched, int ichunkx, int ichunky, int width, float sta=GOTAO_NO_AFFINITY,
                         int nthread=0) 
                         : TAO_PAR_FOR_2D_BASE(a,c,rows,cols,offx,offy,chunkx,chunky,
                                         sched,ichunkx,ichunky,width,sta) {}

                int ndx(int a, int b){ return a*gotao_parfor2D_cols + b; }

                int compute_for2D(int offx, int offy, int chunkx, int chunky)
                {

                    double *in = (double *) gotao_parfor2D_in;
                    double *out = (double *) gotao_parfor2D_out;
                    double diff; double sum=0.0;

                    // global rows and cols
                    int grows = gotao_parfor2D_rows;
                    int gcols = gotao_parfor2D_cols;


                    int xstart = (offx == 0)? 1 : offx;
                    int ystart = (offy == 0)? 1 : offy;
                    int xstop = ((offx + chunkx) >= grows)? grows - 1: offx + chunkx;
                    int ystop = ((offy + chunky) >= gcols)? gcols - 1: offy + chunky;

#if DO_LOI
    kernel_profile_start();
#endif
                    for (int i=xstart; i<xstop; i++) 
                        for (int j=ystart; j<ystop; j++) {
                        out[ndx(i,j)]= 0.25 * (in[ndx(i,j-1)]+  // left
                                               in[ndx(i,j+1)]+  // right
                                               in[ndx(i-1,j)]+  // top
                                               in[ndx(i+1,j)]); // bottom
                               
// for similarity with the OmpSs version, we do not check the residual
//                        diff = out[ndx(i,j)] - in[ndx(i,j)];
//                        sum += diff * diff; 
                        }
#if DO_LOI
    kernel_profile_stop(JACOBI2D);
#if DO_KRD
    for(int x = xstart; x < xstop; x += KRDBLOCKX)
      for(int y = ystart; y < ystop; y += KRDBLOCKY)
      {
      int krdblockx = ((x + KRDBLOCKX - 1) < xstop)? KRDBLOCKX : xstop - x;
      int krdblocky = ((y + KRDBLOCKY - 1) < ystop)? KRDBLOCKY : ystop - y;
      kernel_trace1(JACOBI2D, &in[ndx(x,y)], KREAD(krdblockx*krdblocky)*sizeof(double));
      kernel_trace1(JACOBI2D, &out[ndx(x,y)], KWRITE(krdblockx*krdblocky)*sizeof(double));
      }
#endif
#endif

//                     std::cout << "compute offx " << offx << " offy " << offy 
//                          << " chunkx " << chunkx << " chunky " << chunky 
//                          << " xstart " << xstart << " xstop " << xstop 
//                          << " ystart " << ystart << " ystop " << ystop
//                          << " affinity "  << get_affinity()
//                          << " local residual is " << sum << std::endl;
    return 0;
                }

#if defined(CRIT_PERF_SCHED)
  void set_timetable(int threadid, float ticks, int index){
    time_table[index][threadid] = ticks;
  }

  float get_timetable(int threadid, int index){ 
    float time=0;
    time = time_table[index][threadid];
    return time;
  }
#endif



};


#endif
                 class copy2D : public TAO_PAR_FOR_2D_BASE
                 {
                 public:
#if defined(CRIT_PERF_SCHED)
                  static float time_table[][XITAO_MAXTHREADS];
#endif


                  copy2D(void *a, void*c, int rows, int cols, int offx, int offy, int chunkx, int chunky, 
                   gotao_schedule_2D sched, int ichunkx, int ichunky, int width, float sta=GOTAO_NO_AFFINITY,
                   int nthread=0) 
                  : TAO_PAR_FOR_2D_BASE(a,c,rows,cols,offx,offy,chunkx,chunky,
                   sched,ichunkx,ichunky,width,sta) {}

                  int ndx(int a, int b){ return a*gotao_parfor2D_cols + b; }

                  int compute_for2D(int offx, int offy, int chunkx, int chunky)
                  {
                    double *in = (double *) gotao_parfor2D_in;
                    double *out = (double *) gotao_parfor2D_out;
                    double diff; double sum=0.0;

                    // global rows and cols
                    int grows = gotao_parfor2D_rows;
                    int gcols = gotao_parfor2D_cols;

                    int xstart = offx;
                    int ystart = offy;

                    int xstop = ((offx + chunkx) >= grows)? grows: offx + chunkx;
                    int ystop = ((offy + chunky) >= gcols)? gcols: offy + chunky;
#if DO_LOI
                    kernel_profile_start();
#endif

                    for (int i=xstart; i<xstop; i++) 
                      for (int j=ystart; j<ystop; j++) 
                       out[ndx(i,j)]= in[ndx(i,j)];

//                    std::cout << "copy: offx " << offx << " offy " << offy 
//                          << " chunkx " << chunkx << " chunky " << chunky 
//                          << " xstart " << xstart << " xstop " << xstop 
//                          << " ystart " << ystart << " ystop " << ystop
//                          << " affinity "  << get_affinity()  
//                          << std::endl;
#if DO_LOI
                     kernel_profile_stop(COPY2D);
#if DO_KRD
                     for(int x = xstart; x < xstop; x += KRDBLOCKX)
                      for(int y = ystart; y < ystop; y += KRDBLOCKY)
                      {
                        int krdblockx = ((x + KRDBLOCKX - 1) < xstop)? KRDBLOCKX : xstop - x;
                        int krdblocky = ((y + KRDBLOCKY - 1) < ystop)? KRDBLOCKY : ystop - y;
                        kernel_trace1(JACOBI2D, &in[ndx(x,y)], KREAD(krdblockx*krdblocky)*sizeof(double));
                        kernel_trace1(JACOBI2D, &out[ndx(x,y)], KWRITE(krdblockx*krdblocky)*sizeof(double));
                      }
#endif
#endif
                      return 0;
                    }

#if defined(CRIT_PERF_SCHED)
                    void set_timetable(int threadid, float ticks, int index){
                      time_table[index][threadid] = ticks;
                    }

                    float get_timetable(int threadid, int index){ 
                      float time=0;
                      time = time_table[index][threadid];
                      return time;
                    }
#endif



                  };

/*
 * Blocked Jacobi solver: one iteration step
 */
/*double relax_jacobi (unsigned sizey, double (*u)[sizey], double (*utmp)[sizey], unsigned sizex)
{
    double diff, sum=0.0;
    int nbx, bx, nby, by;
  
    nbx = 4;
    bx = sizex/nbx;
    nby = 4;
    by = sizey/nby;
    for (int ii=0; ii<nbx; ii++)
        for (int jj=0; jj<nby; jj++) 
            for (int i=1+ii*bx; i<=min((ii+1)*bx, sizex-2); i++) 
                for (int j=1+jj*by; j<=min((jj+1)*by, sizey-2); j++) {
                utmp[i][j]= 0.25 * (u[ i][j-1 ]+  // left
                         u[ i][(j+1) ]+  // right
                             u[(i-1)][ j]+  // top
                             u[ (i+1)][ j ]); // bottom
                diff = utmp[i][j] - u[i][j];
                sum += diff * diff; 
            }

    return sum;
}
*/
/*
 * Blocked Red-Black solver: one iteration step
 */
/*
double relax_redblack (unsigned sizey, double (*u)[sizey], unsigned sizex )
{
    double unew, diff, sum=0.0;
    int nbx, bx, nby, by;
    int lsw;

    nbx = 4;
    bx = sizex/nbx;
    nby = 4;
    by = sizey/nby;
    // Computing "Red" blocks
    for (int ii=0; ii<nbx; ii++) {
        lsw = ii%2;
        for (int jj=lsw; jj<nby; jj=jj+2) 
            for (int i=1+ii*bx; i<=min((ii+1)*bx, sizex-2); i++) 
                for (int j=1+jj*by; j<=min((jj+1)*by, sizey-2); j++) {
                unew= 0.25 * (    u[ i][ (j-1) ]+  // left
                      u[ i][(j+1) ]+  // right
                      u[ (i-1)][ j ]+  // top
                      u[ (i+1)][ j ]); // bottom
                diff = unew - u[i][j];
                sum += diff * diff; 
                u[i][j]=unew;
            }
    }

    // Computing "Black" blocks
    for (int ii=0; ii<nbx; ii++) {
        lsw = (ii+1)%2;
        for (int jj=lsw; jj<nby; jj=jj+2) 
            for (int i=1+ii*bx; i<=min((ii+1)*bx, sizex-2); i++) 
                for (int j=1+jj*by; j<=min((jj+1)*by, sizey-2); j++) {
                unew= 0.25 * (    u[ i][ (j-1) ]+  // left
                      u[ i][ (j+1) ]+  // right
                      u[ (i-1)][ j     ]+  // top
                      u[ (i+1)][ j     ]); // bottom
                diff = unew - u[i][ j];
                sum += diff * diff; 
                u[i][j]=unew;
            }
    }

    return sum;
}
*/
/*
 * Blocked Gauss-Seidel solver: one iteration step
 */
/*
double relax_gauss (unsigned padding, unsigned sizey, double (*u)[sizey], unsigned sizex )
{
    double unew, diff, sum=0.0;
    int nbx, bx, nby, by;

    nbx = 8;
    bx = sizex/nbx;
    nby = 8;
    by = sizey/nby;
    for (int ii=0; ii<nbx; ii++)
        for (int jj=0; jj<nby; jj++){
            for (int i=1+ii*bx; i<=min((ii+1)*bx, sizex-2); i++)
                for (int j=1+jj*by; j<=min((jj+1)*by, sizey-2); j++) {
                unew= 0.25 * (    u[ i][ (j-1) ]+  // left
                      u[ i][(j+1) ]+  // right
                      u[ (i-1)][ j     ]+  // top
                      u[ (i+1)][ j     ]); // bottom
                diff = unew - u[i][ j];
                sum += diff * diff; 
                u[i][j]=unew;
                }
        }

    return sum;
}
*/
