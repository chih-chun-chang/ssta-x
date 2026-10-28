#pragma once

#include <cuda.h>
#include <cuda_runtime.h>
#include <ssta/cell/cell.hpp>
#include <ssta/cuda/rct.cuh>

struct TimingCUDA {
  // cell
  int* cell_type;
  int* cell_hasclk;
  int* cell_level_ptr;
  int* cell_blevel_ptr;
  int* map_to_forward;
  // tcell data
  cell::TimingSense* tcell_timing_sense;
  float* tcell_timing_data_slew;  // 18
  float* tcell_timing_data_delay; //  18
  float* tcell_constraint_setup_data; // 6
  cell::EdgeType* tcell_constraint_hold_type;
  float* tcell_constraint_hold_data; // 6
  int* tcell_timing_ptr;
  int* tcell_constraint_ptr;
  // cell data
  cell::TimingSense* cell_arch_sense;
  float* cell_arch_data_slew; // 18
  float* cell_arch_data_delay; // 18
  int* cell_arch_ptr;
  // node
  int* ipin;
  int* ipin_ptr;
  float* ipin_timing_data_slew; // 32
  float* ipin_timing_data_at; // 32
  float* ipin_timing_data_rat; // 32
  float* ipint_timing_data_slew; // 32
  float* ipint_timing_data_at; // 32
  float* ipint_timing_data_rat; // 32
  float* opin_timing_data_slew; // 32 
  float* opin_timing_data_at; // 32
  float* opin_timing_data_rat; // 32
  float* output_timing_data_slew; // 32
  float* output_timing_data_at; // 32
  float* output_timing_data_rat; // 32
  int* wp;
  int* nipin;
  int* nipin_ptr;
  int* bnipin_ptr;
  int* bnipin_ptr_each_gate;
  int* opin;
  int* opin_ptr;
  int* ipint_ptr;
  int* opin_index; // to correct lookup the opin
};

void _addWireParamsForward(TimingCUDA& timing_cuda, RctCUDA& rct, int num_inputs, int start_idx, int num_pins, int level);
void _addWireParamsBackward(TimingCUDA& timing_cuda, RctCUDA& rct, int num_inputs, int start_idx, int num_gates, int level);
void _getTDataSense(TimingCUDA& timing_cuda, int num_inputs, int num_gates, int level, int dir);
void _addDelayParam(TimingCUDA& timing_cuda, RctCUDA& rct, int num_inputs, int num_gates, int level, int dir, float clockTP);
void _max(TimingCUDA& timing_cuda, int num_inputs, int num_gates, int level);
void _maxR(TimingCUDA& timing_cuda, int num_inputs, int num_gates, int level);

__global__ void addWireParamsForward(TimingCUDA timing, RctCUDA rct, int num_inputs, int start_ipin, int num_pins, int level);
__global__ void getTDataSense(TimingCUDA timing, int num_inputs, int num_gates, int level, int dir);
__global__ void addDelayParam(TimingCUDA timing, RctCUDA rct, int num_inputs, int num_gates, int level, int dir, float clockTP);
__global__ void Max(const TimingCUDA timing, int num_inputs, int num_gates, int level);
__global__ void addWireParamsBackward(TimingCUDA timing, RctCUDA rct, int num_inputs, int start_ipin, int num_pins, int level);
__global__ void MaxR(const TimingCUDA timing, int num_inputs, int num_gates, int level);

__device__ inline float reduce_sum_8_lanes(unsigned mask, float val);
// centered: variance without E[max^2] - c^2, for required times (means near the clock period)
__device__ void max_kernel(float* Cc, float* Aa, float* Bb, int print=0, bool centered=false);
