#include "synthmat.h"
#include <iostream>
#include <chrono>
using namespace std; 
int main(int argc, char** argv) {
  int size = 64; 
  if(argc > 1) size = atoi(argv[1]);
  std::cout << "Matrix size is " << size << " x " << size << std::endl;
  Synth_MatMul matmul(size, 1);  
  auto t1 = std::chrono::system_clock::now();
  matmul.execute(0);
  auto t2 = std::chrono::system_clock::now();
  std::chrono::duration<double> time = t2 - t1; 
  std::cout << "Time taken: " << time.count() << " s" << std::endl;
  return 0;
}
