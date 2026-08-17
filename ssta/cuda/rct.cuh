#pragma once

#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>

struct RctCUDA
{
  // input
  int num_nets;
  int num_nodes;
  int num_edges;
  int* nodes_st;
  int* edges_st;
  int* edges_s;
  int* edges_t;
  float* ress;
  float* caps;
  int* ports;
  int* taps_st;
  int* taps;
  float* tapCaps;
  // bfs
  int* distances;
  int* counts;
  int* orders;
  int* outputs;
  int* pid;
  float* pres;
  // timing
  float Mc; // Metal Capacitance Scalar
  float Mr; // Metal Resistance Scalar 

  float* load;
  float* delay;
  float* beta;
  float* slew;
  float* tapDelay;
  float* tapSlew;
  float* outCap;
};

void _bfs(RctCUDA& rct_cuda);
void _compute_rc_timing(RctCUDA& rct_cuda, float Mc, float Mr);
void _compute_rc_timing2(RctCUDA& rct_cuda, float Mc, float Mr);
void _compute_rc_timing_block(RctCUDA& rct_cuda, float Mc, float Mr);

#define CUDA_CHECK(err) __cuda_check(err, __FILE__, __LINE__)

inline void __cuda_check(cudaError_t err, const char* file, const int line) {
  if (err != cudaSuccess) {
    std::cerr << "CUDA error in " << file << ":" << line
              << ": " << cudaGetErrorString(err) << std::endl;
    // You might want to exit or throw an exception here
    exit(EXIT_FAILURE);
  }
}