#include <ssta/cuda/timing.cuh>
#include <ssta/cuda/tuning_macros.cuh>

// launch #warp to add from opin to tap (nipin)
// a warp do a pin
// 0-7  fall early
// 8-15 fall late
// 16-23 rise early
// 24-31 rise late
__global__ void addWireParamsForward(
  TimingCUDA timing, RctCUDA rct, int num_inputs, int start_ipin, int num_pins, int level) 
{  
  int tid     = blockIdx.x*blockDim.x + threadIdx.x;
  int warp_id  = tid >> 5;
  int lane_id = threadIdx.x & 31;
  if (warp_id >= num_pins) return;

  int mode    = lane_id >> 3;
  int sens    = lane_id & 7;

  int pin_idx;
  float at, in, in0;
  int port_id;
  int nipin_beg;
  if (level == -1) { // input
    pin_idx  =  warp_id;
    at  = timing.ipin_timing_data_at[pin_idx*32 + lane_id];
    in  = timing.ipin_timing_data_slew[pin_idx*32 + lane_id];
    in0 = timing.ipin_timing_data_slew[pin_idx*32 + 8*mode];
    port_id  = timing.wp[pin_idx];
    nipin_beg = timing.nipin_ptr[pin_idx];
  } 
  else {
    // one warp per output pin of the level (a cell may have several)
    int opin_st = timing.opin_ptr[timing.cell_level_ptr[level]];
    pin_idx = timing.opin[opin_st + warp_id];
    at  = timing.opin_timing_data_at[pin_idx*32 + lane_id];
    in  = timing.opin_timing_data_slew[pin_idx*32 + lane_id];
    in0 = timing.opin_timing_data_slew[pin_idx*32 + 8*mode];
    port_id  = timing.wp[pin_idx+num_inputs];
    nipin_beg = timing.nipin_ptr[start_ipin + warp_id];
  }
  if (port_id < 0) return;  // no RC net (an unused input): no taps to drive

  // Figure out its wire‐port and taps  
  int tap_st   = rct.taps_st[port_id];
  int tap_en   = rct.taps_st[port_id+1];
  int num_taps = tap_en - tap_st;


  float out_at, out_slew;
  for (int i = 0; i < num_taps; ++i) {
    int tap = tap_st + i;
    out_at = at;
    out_slew = 0.0f;
    // at
    if (sens == 0) {
      out_at += rct.tapDelay[4*tap + (mode/2)];
    }
    if (sens == 6) {
      out_at += rct.tapDelay[4*tap + (mode/2) + 2];
    }
    // slew
    float slew0 = rct.tapSlew[4*tap + (lane_id/16)];
    float slew1 = rct.tapSlew[4*tap + (lane_id/16) + 2];
    if (sens == 0) {
      out_slew = sqrtf(in*in + slew0*slew0);
    }
    if (sens >= 1 && sens <= 5) {
      out_slew = (sqrtf((in0 + 3*in)*(in0 + 3*in) + slew0*slew0) -
             sqrtf((in0 - 3*in)*(in0 - 3*in) + slew0*slew0)) / 6;
    }
    if (sens == 6) {
      float out0 = sqrtf(in0*in0 + slew0*slew0);
      out_slew = (sqrtf((in0 + 3*in)*(in0 + 3*in) + (slew0 + 3*slew1)*(slew0 + 3*slew1)) - out0) / 3;
    }
    if (sens == 7) {
      out_slew = (sqrtf((in0 + 3*in)*(in0 + 3*in) + slew0*slew0) -
             sqrtf((in0 - 3*in)*(in0 - 3*in) + slew0*slew0)) / 6;
    }

    int nipin_idx = timing.nipin[nipin_beg+ i];
    timing.ipin_timing_data_at[nipin_idx*32 + lane_id] = out_at;
    timing.ipin_timing_data_slew[nipin_idx*32 + lane_id] = out_slew;
  }
  
} // addWireParamsForward


__global__ void addWireParamsBackward(
  TimingCUDA timing, RctCUDA rct, int num_inputs, int start_idx, int num_gates, int level) 
{  
  __shared__ float cur_rat[32], next_rat[32];

  int gate_idx = blockIdx.x;
  int lane_id = threadIdx.x & 31;
  if (gate_idx >= num_gates) return;

  int mode    = lane_id >> 3;
  int sens    = lane_id & 7;
  
  int pin_idx;
  int port_id;
  int nipin_beg;
  if (level == -1) { // input
    pin_idx  =  gate_idx;
    port_id  = timing.wp[pin_idx];
    nipin_beg = timing.nipin_ptr[pin_idx];
    if (port_id < 0) return;  // no RC net (an unused input)

    // Figure out its wire‐port and taps  
    int tap_st   = rct.taps_st[port_id];
    int tap_en   = rct.taps_st[port_id+1];
    int num_taps = tap_en - tap_st;

    bool isLate = ((mode & 1) == 1); 
    float rat;
    for (int i = 0; i < num_taps; ++i) {
      int tap = tap_st + i;
      int nipin_idx = timing.nipin[nipin_beg+ i];
      rat = timing.ipin_timing_data_rat[nipin_idx*32 + lane_id];
      if (sens == 0) {
        rat -= rct.tapDelay[4*tap + (mode/2)];
      }
      if (sens == 6) {
        rat -= rct.tapDelay[4*tap + (mode/2) + 2];
      }

     if (i == 0) {
          cur_rat[lane_id] = rat;
          __syncwarp();
        }
        else {
          next_rat[lane_id] = rat;
          __syncwarp();
          if (isLate) {
            cur_rat[lane_id] = -cur_rat[lane_id];
            next_rat[lane_id] = -next_rat[lane_id];
          }
          __syncwarp();
          max_kernel(cur_rat, cur_rat, next_rat);
          __syncwarp();
          if (isLate) cur_rat[lane_id] = -cur_rat[lane_id];
        }

    }
    rat = cur_rat[lane_id];
    timing.ipin_timing_data_rat[gate_idx*32 + lane_id] = rat;
  } 
  else {
    int cell_ptr = timing.map_to_forward[timing.cell_blevel_ptr[level] + gate_idx];
    int has_clk   = timing.cell_hasclk[cell_ptr];
    if (has_clk) return;

    int opin_st = timing.opin_ptr[cell_ptr];
    int opin_en = timing.opin_ptr[cell_ptr + 1];
    int numop = opin_en - opin_st;
    int bnipin_ptr_each_gate = timing.bnipin_ptr_each_gate[start_idx + gate_idx];
    for (int i = 0; i < numop; ++i) {
      pin_idx = timing.opin[opin_st + i];
      port_id  = timing.wp[pin_idx+num_inputs];
      nipin_beg = timing.bnipin_ptr[bnipin_ptr_each_gate + i];
      if (port_id < 0) continue;  // output pin without an RC net
      
      int tap_st   = rct.taps_st[port_id];
      int tap_en   = rct.taps_st[port_id+1];
      int num_taps = tap_en - tap_st;

      bool isLate = ((mode & 1) == 1); 
      float rat;
      for (int j = 0; j < num_taps; ++j) {
        int tap = tap_st + j;
        int nipin_idx = timing.nipin[nipin_beg + j];
        rat = timing.ipin_timing_data_rat[nipin_idx*32 + lane_id];
        if (sens == 0) {
          rat -= rct.tapDelay[4*tap + (mode/2)];
        }
        if (sens == 6) {
          rat -= rct.tapDelay[4*tap + (mode/2) + 2];
        }

        if (j == 0) {
          cur_rat[lane_id] = rat;
          __syncwarp();
        }
        else {
          next_rat[lane_id] = rat;
          __syncwarp();
          if (isLate) {
            cur_rat[lane_id] = -cur_rat[lane_id];
            next_rat[lane_id] = -next_rat[lane_id];
          }
          __syncwarp();
          max_kernel(cur_rat, cur_rat, next_rat);
          __syncwarp();
          if (isLate) cur_rat[lane_id] = -cur_rat[lane_id];
        }
      }
      rat = cur_rat[lane_id];
      timing.opin_timing_data_rat[pin_idx*32 + lane_id] = rat;
    }
  }
} // addWireParamsBackward



__device__ inline float reduce_sum_8_lanes(unsigned mask, float val) {
  val += __shfl_down_sync(mask, val, 4); // Lanes 0..3 get sum from 4..7 added
  val += __shfl_down_sync(mask, val, 2); // Lanes 0,1 get sum from 2,3 added
  val += __shfl_down_sync(mask, val, 1); // Lane 0 gets sum from 1 added
  return val;
}

// launch #warp to get arch sense and add delay
// a warp do a pin
// type = 0 : 0-7  fall early
// type = 1 : 8-15 fall late
// type = 2 : 16-23 rise early
// type = 3 : 24-31 rise late
__global__ void getTDataSense(
  TimingCUDA timing, int num_inputs, int num_gates, int level, int dir) 
{
  int gate_idx = blockIdx.x;
  if (gate_idx >= num_gates) return;

  const int MAX_WARPS_PER_BLOCK = 16;
  __shared__ float sh_at_A[MAX_WARPS_PER_BLOCK * 32];
  __shared__ float sh_at_B[MAX_WARPS_PER_BLOCK * 32];
  __shared__ float sh_sl_A[MAX_WARPS_PER_BLOCK * 32];
  __shared__ float sh_sl_B[MAX_WARPS_PER_BLOCK * 32];

  int cell_ptr;
  if (dir != 0) { // backward
    cell_ptr = timing.map_to_forward[timing.cell_blevel_ptr[level] + gate_idx];
  }
  else {
    cell_ptr = timing.cell_level_ptr[level] + gate_idx;
  }
  int cell_type = timing.cell_type[cell_ptr];
  int has_clk   = timing.cell_hasclk[cell_ptr];

  int ipin_st = timing.ipin_ptr[cell_ptr + num_inputs];
  int ipin_en = timing.ipin_ptr[cell_ptr + num_inputs + 1];
  int numip = ipin_en - ipin_st;

  int opin_st = timing.opin_ptr[cell_ptr];
  int opin_en = timing.opin_ptr[cell_ptr + 1];
  int numop = opin_en - opin_st;

  int ipint_st = timing.ipint_ptr[cell_ptr];

  int warp_id = threadIdx.x >> 5;
  bool active = (warp_id < numop);
  unsigned mask = __ballot_sync(0xFFFFFFFF, active);
  int lane_id = threadIdx.x & 31;
  int type = lane_id >> 3;

  if (active) {
  
  // library index of this warp's output pin: arc tables are per (lib output, lib input)
  int o_lib = timing.opin_index[opin_st + warp_id];

  if (has_clk) {
    int cell_idx = timing.tcell_timing_ptr[cell_type] + o_lib;
    cell::TimingSense pinsense = timing.tcell_timing_sense[cell_idx];
    int ipin = timing.ipin[ipin_st];
    if (dir == 0) {
      float at   = timing.ipin_timing_data_at[ipin*32 + lane_id];
      float slew = timing.ipin_timing_data_slew[ipin*32 + lane_id];
      if (pinsense == cell::TimingSense::PositiveUnate) 
      {
      }
      else if (pinsense == cell::TimingSense::NegativeUnate) 
      {
        int partner = lane_id ^ 16;
        at   = __shfl_sync(0xFFFFFFFF, at, partner);
        slew = __shfl_sync(0xFFFFFFFF, slew, partner);    
      }
      else if (pinsense == cell::TimingSense::NonUnate) 
      {
        int sh_idx = warp_id * 32 + lane_id;
        sh_at_A[sh_idx] = at;
        sh_sl_A[sh_idx] = slew;

        int partner = lane_id ^ 16;
        sh_at_B[sh_idx] = __shfl_sync(mask, at, partner);
        sh_sl_B[sh_idx] = __shfl_sync(mask, slew, partner);
        __syncwarp(mask);

        float* p_at_A = &sh_at_A[warp_id * 32];
        float* p_at_B = &sh_at_B[warp_id * 32];
        float* p_sl_A = &sh_sl_A[warp_id * 32];
        float* p_sl_B = &sh_sl_B[warp_id * 32];

        bool is_early = (type == 0) || (type == 2);
        if (is_early) // min(A, B) = -max(-A, -B)
        {
          p_at_A[lane_id] = -p_at_A[lane_id];
          p_at_B[lane_id] = -p_at_B[lane_id];
          p_sl_A[lane_id] = -p_sl_A[lane_id];
          p_sl_B[lane_id] = -p_sl_B[lane_id];
        }
        __syncwarp(mask);
        max_kernel(p_at_A, p_at_A, p_at_B);
        max_kernel(p_sl_A, p_sl_A, p_sl_B);
        __syncwarp(mask);
        if (is_early) 
        {
          p_at_A[lane_id] = -p_at_A[lane_id];
          p_sl_A[lane_id] = -p_sl_A[lane_id];
        }
        __syncwarp(mask);
        at   = sh_at_A[sh_idx];
        slew = sh_sl_A[sh_idx];
      }

      int timing_ptr = ipint_st * 32 + (warp_id * numip * 32) + lane_id;
      timing.ipint_timing_data_at[timing_ptr]   = at;
      timing.ipint_timing_data_slew[timing_ptr] = slew;
    }
    else { // backward
    } 
  } // clk cell end
  else { 
    int cell_idx = timing.cell_arch_ptr[cell_type] + o_lib*numip;
    for (int i = 0; i < numip; ++i) {
      int ipin = timing.ipin[ipin_st + i];

      cell::TimingSense pinsense = timing.cell_arch_sense[cell_idx + i];
      if (dir == 0 || dir == 2) // Forward
      {
        float at   = timing.ipin_timing_data_at[ipin*32 + lane_id];
        float slew = timing.ipin_timing_data_slew[ipin*32 + lane_id];
        if (pinsense == cell::TimingSense::PositiveUnate) 
        {
          //continue;
        }
        else if (pinsense == cell::TimingSense::NegativeUnate) 
        {
          int partner = lane_id ^ 16;
          at   = __shfl_sync(0xFFFFFFFF, at, partner);
          slew = __shfl_sync(0xFFFFFFFF, slew, partner);    
        }
        else if (pinsense == cell::TimingSense::NonUnate) 
        {
          int sh_idx = warp_id * 32 + lane_id;
          sh_at_A[sh_idx] = at;
          sh_sl_A[sh_idx] = slew;

          int partner = lane_id ^ 16;
          sh_at_B[sh_idx] = __shfl_sync(0xFFFFFFFF, at, partner);
          sh_sl_B[sh_idx] = __shfl_sync(0xFFFFFFFF, slew, partner);
          __syncwarp(mask);

          float* p_at_A = &sh_at_A[warp_id * 32];
          float* p_at_B = &sh_at_B[warp_id * 32];
          float* p_sl_A = &sh_sl_A[warp_id * 32];
          float* p_sl_B = &sh_sl_B[warp_id * 32];

          bool is_early = (type == 0) || (type == 2);
          if (is_early) // min(A, B) = -max(-A, -B)
          {
            p_at_A[lane_id] = -p_at_A[lane_id];
            p_at_B[lane_id] = -p_at_B[lane_id];
            p_sl_A[lane_id] = -p_sl_A[lane_id];
            p_sl_B[lane_id] = -p_sl_B[lane_id];
          }
            __syncwarp();

            max_kernel(p_at_A, p_at_A, p_at_B);
            max_kernel(p_sl_A, p_sl_A, p_sl_B);
            __syncwarp();
          if (is_early)
          {
            p_at_A[lane_id] = -p_at_A[lane_id];
            p_sl_A[lane_id] = -p_sl_A[lane_id];
          }
          __syncwarp(mask);
          at   = sh_at_A[sh_idx];
          slew = sh_sl_A[sh_idx];
        }
        int timing_ptr = ipint_st * 32 + (warp_id * numip * 32) + i * 32 + lane_id;
        timing.ipint_timing_data_at[timing_ptr]   = at;
        timing.ipint_timing_data_slew[timing_ptr] = slew;
      }
      else // Backward
      {
        int timing_ptr = ipint_st * 32 + (warp_id * numip * 32) + i * 32 + lane_id;
        float rat = timing.ipint_timing_data_rat[timing_ptr];
        if (pinsense == cell::TimingSense::PositiveUnate) 
        {
          //continue;
        }
        else if (pinsense == cell::TimingSense::NegativeUnate) 
        {
          int partner = lane_id ^ 16;
          rat  = __shfl_sync(0xFFFFFFFF, rat, partner);
        }
        else if (pinsense == cell::TimingSense::NonUnate) 
        {
          int sh_idx = warp_id * 32 + lane_id;
          sh_at_A[sh_idx] = rat;

          int partner = lane_id ^ 16;
          sh_at_B[sh_idx] = __shfl_sync(0xFFFFFFFF, rat, partner);

          float* p_at_A = &sh_at_A[warp_id * 32];
          float* p_at_B = &sh_at_B[warp_id * 32];

          bool is_late = (type == 1) || (type == 3);
          if (is_late) // min(A, B) = -max(-A, -B)
          {
            p_at_A[lane_id] = -p_at_A[lane_id];
            p_at_B[lane_id] = -p_at_B[lane_id];
          }
          __syncwarp();
          max_kernel(p_at_A, p_at_A, p_at_B);
          __syncwarp();  // the root lane wrote the random term of lane root+7
          if (is_late) 
          {
            p_at_A[lane_id] = -p_at_A[lane_id];
          }
          __syncwarp(mask);
          rat   = sh_at_A[sh_idx];
        }
        timing.ipint_timing_data_rat[timing_ptr] = rat;
      }
    }
  }

  } // if (active)

}

// dir == 0 → Forward, dir == 1 → Reverse
__global__ void addDelayParam(
  TimingCUDA timing, RctCUDA rct, int num_inputs, int num_gates, int level, int dir, float clockTP)
{
  int gate_idx = blockIdx.x;
  if (gate_idx >= num_gates) return;

  int cell_ptr;
  if (dir != 0) { // backward
    cell_ptr = timing.map_to_forward[timing.cell_blevel_ptr[level] + gate_idx];
  }
  else {
    cell_ptr = timing.cell_level_ptr[level] + gate_idx;
  }
  int cell_type = timing.cell_type[cell_ptr];
  int has_clk   = timing.cell_hasclk[cell_ptr];

  int ipin_st = timing.ipin_ptr[cell_ptr + num_inputs];
  int ipin_en = timing.ipin_ptr[cell_ptr + num_inputs + 1];
  int numip = ipin_en - ipin_st;

  int opin_st = timing.opin_ptr[cell_ptr];
  int opin_en = timing.opin_ptr[cell_ptr + 1];
  int numop = opin_en - opin_st;

  int ipint_st = timing.ipint_ptr[cell_ptr];

  // one warp per opin
  int warp_id = threadIdx.x >> 5;
  if (warp_id >= numop) return;
  int out_p = timing.opin[opin_st + warp_id];
  int o_lib = timing.opin_index[opin_st + warp_id];  // arc tables are per (lib output, lib input)

  int lane_id = threadIdx.x & 31;
  int type = lane_id >> 3;
  int type_id = lane_id & 7;

  if (has_clk) {
    if (dir == 0) {
      int cell_idx = (timing.tcell_timing_ptr[cell_type] + o_lib)*18;
      int timing_ptr = ipint_st * 32 + (warp_id * numip * 32) + lane_id; 

      float at = timing.ipint_timing_data_at[timing_ptr];
      float sl = timing.ipint_timing_data_slew[timing_ptr];

      int wire_id  = timing.wp[out_p+num_inputs];  // the net this output pin drives
      const float no_load[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // output pin without an RC net
      const float* cap = wire_id < 0 ? no_load : &rct.outCap[4*wire_id];

      float* dep = &timing.tcell_timing_data_delay[cell_idx];
      float* slp = &timing.tcell_timing_data_slew[cell_idx];
    
      int base = (type/2)*9;
      float d0 = dep[base+0], d1 = dep[base+1], d2 = dep[base+2];
      float s0 = slp[base+0], s1 = slp[base+1], s2 = slp[base+2];

      float p = 0.0f, so = 0.0f;
      switch (type_id) {
        case 0: // nominal  out[0] = dep[0] + dep[1]*cap[0] + dep[2]*slew[0];
          p  = d0 + d1 * cap[type/2] + d2 * sl;
          so = s0 + s1 * cap[type/2] + s2 * sl;
          break;
        case 6: // metal‐load sensitivity out[6] = dep[1] * cap[1];
          p  = d1 * cap[type/2 + 2];
          so = s1 * cap[type/2 + 2];
          break;
        case 7: // random‐variation out[7] = std::sqrt((dep[0]*dep[8])*(dep[0]*dep[8]) + (dep[2]*slew[7])*(dep[2]*slew[7]));
          p  = sqrtf((d0*dep[base+8])*(d0*dep[base+8]) + (d2*sl)*(d2*sl));
          so = sqrtf((s0*slp[base+8])*(s0*slp[base+8]) + (s2*sl)*(s2*sl));
          break;
        default: // general sensitivity 1–5 out[i] = dep[0]*dep[i+2] + dep[2]*slew[i]; // general component
          p  = d0 * dep[base+type_id+2] + d2 * sl;
          so = s0 * slp[base+type_id+2] + s2 * sl;
          break;
      }
      timing.ipint_timing_data_at   [timing_ptr] = at + p;
      timing.ipint_timing_data_slew [timing_ptr] = so;
    }
    else {
      // setup/hold checks belong to the cell, not to an output pin: one warp does them
      if (warp_id != 0) return;
      int ipin = timing.ipin[ipin_st];
      int timing_ptr = ipin * 32 + lane_id; // ck
      float sck = timing.ipin_timing_data_slew[timing_ptr];
      float at = timing.ipin_timing_data_at[timing_ptr];

      for (int i = 1; i < numip; ++i) { // one warp per opin -> one warp has to iterate all ipins
        float sin = timing.ipin_timing_data_slew[timing_ptr + i * 32];
        int cell_idx = (timing.tcell_constraint_ptr[cell_type] + i - 1)*6;
        // type = 0 : 0-7  fall early  (hold fall) sck: rise late
        // type = 1 : 8-15 fall late   (setup fall) sck: rise early
        // type = 2 : 16-23 rise early (hold rise) sck: rise late
        // type = 3 : 24-31 rise late  (setup rise) sck: rise early
        float* dep;
        int partner = lane_id;
        switch (type) {
          case 0:
            partner = lane_id + 24;   // 0-7 <-> 24-31
            dep = &timing.tcell_constraint_hold_data[cell_idx];
            break;
          case 1: 
            partner = lane_id + 8;    // 8-15 <-> 16-23
            dep = &timing.tcell_constraint_setup_data[cell_idx];
            break;
          case 2: 
            partner = lane_id + 8;    // 16-23 <-> 24-31
            dep = &timing.tcell_constraint_hold_data[cell_idx + 3];
            break;
          case 3: 
            partner = lane_id - 8;   // 24-31 <-> 16-23
            dep = &timing.tcell_constraint_setup_data[cell_idx + 3];
            break;
          default:
            break;
        }
        __syncwarp();
        // the clock lane of this mode; sck and at keep the clock's own lanes for the next input
        float sck_p = __shfl_sync(0xffffffff, sck, partner, 32);
        float d0 = dep[0], d1 = dep[1], d2 = dep[2];
        float out = 0.0f;
        switch (type_id) {
          case 0: // nominal  out[0] = dep[0] + dep[1]*sck[0] + dep[2]*sin[0];
            out = d0 + d1*sck_p + d2*sin;
            break;
          case 7: // random‐variation out[7] = std::sqrt(sck[7]*sck[7] + sin[7]*sin[7]); 
            out = sqrtf(sck_p*sck_p + sin*sin);
            break;
          default: // general sensitivity 1–6 out[i] = dep[1]*sck[i] + dep[2]*sin[i]; // general component
            out = d1*sck_p + d2*sin;
            break;
        }
        
        __syncwarp();
        float at_p = __shfl_sync(0xffffffff, at, partner, 32);
        switch (type) {
          case 0: // fall early _ADD(inPtr->ratFallEarly, instance->cpin->tData.atRiseLate, tq);
            out += at_p;
            break;
          case 1: // fall late  _SUB(inPtr->ratFallLate, instance->cpin->tData.atRiseEarly, tq);
            out = at_p - out;
            //inPtr->ratFallLate[0] += clockTP; // rat late calculation
            if (type_id == 0) out += clockTP;
            break;
          case 2: // rise early _ADD(inPtr->ratRiseEarly, instance->cpin->tData.atRiseLate, tq);
            out += at_p;
            break;
          case 3: // rise late  _SUB(inPtr->ratRiseLate, instance->cpin->tData.atRiseEarly, tq);
            out = at_p - out;
            //inPtr->ratRiseLate[0] += clockTP; // rat late calculation
            if (type_id == 0) out += clockTP;
            break;
          default:
            break;
            
        }
        __syncwarp();
        timing.ipin_timing_data_rat[timing_ptr + i * 32] = out;
      } // for ipin
    }
  } // clk cell
  else {
    int cell_idx = (timing.cell_arch_ptr[cell_type] + o_lib*numip)*18;
    for (int i = 0; i < numip; ++i) {
      int timing_ptr = ipint_st * 32 + (warp_id * numip * 32) + i * 32 + lane_id;

      float at = timing.ipint_timing_data_at[timing_ptr];
      float sl = timing.ipint_timing_data_slew[timing_ptr];

      int wire_id  = timing.wp[out_p+num_inputs];  // the net this output pin drives
      const float no_load[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // output pin without an RC net
      const float* cap = wire_id < 0 ? no_load : &rct.outCap[4*wire_id];

      float* dep = &timing.cell_arch_data_delay[cell_idx + i*18];
      float* slp = &timing.cell_arch_data_slew[cell_idx + i*18];
    
      int base = (type/2)*9;
      float d0 = dep[base+0], d1 = dep[base+1], d2 = dep[base+2];
      float s0 = slp[base+0], s1 = slp[base+1], s2 = slp[base+2];

      float p = 0.0f, so = 0.0f;
      switch (type_id) {
        case 0: // nominal  out[0] = dep[0] + dep[1]*cap[0] + dep[2]*slew[0];
          p  = d0 + d1 * cap[type/2] + d2 * sl;
          so = s0 + s1 * cap[type/2] + s2 * sl;
          break;
        case 6: // metal‐load sensitivity out[6] = dep[1] * cap[1];
          p  = d1 * cap[type/2 + 2];
          so = s1 * cap[type/2 + 2];
          break;
        case 7: // random‐variation out[7] = std::sqrt((dep[0]*dep[8])*(dep[0]*dep[8]) + (dep[2]*slew[7])*(dep[2]*slew[7]));
          p  = sqrtf((d0*dep[base+8])*(d0*dep[base+8]) + (d2*sl)*(d2*sl));
          so = sqrtf((s0*slp[base+8])*(s0*slp[base+8]) + (s2*sl)*(s2*sl));
          break;
        default: // general sensitivity 1–5 out[i] = dep[0]*dep[i+2] + dep[2]*slew[i]; // general component
          p  = d0 * dep[base+type_id+2] + d2 * sl;
          so = s0 * slp[base+type_id+2] + s2 * sl;
          break;
      }

      if (dir == 0) {
        timing.ipint_timing_data_at   [timing_ptr] = at + p;
        timing.ipint_timing_data_slew [timing_ptr] = so;
      }
      else {
        float rat = timing.opin_timing_data_rat[out_p*32 + lane_id];
        timing.ipint_timing_data_rat[timing_ptr] = rat - p;
      }
    }
  }


}


__device__ void max_kernel(float* Cc, float* Aa, float* Bb, int print) {
  int laneId = threadIdx.x & 31;
  int typeId = laneId % 8;
  if (laneId > 31) return;

  //int type = laneId >> 3;
  // Calculate the mask for the 8-lane segment this thread belongs to
  // e.g., lane 5 -> laneId/8 = 0 -> 0xFF << 0 = 0x00FF
  // e.g., lane 10 -> laneId/8 = 1 -> 0xFF << 8 = 0xFF00
  unsigned segmentMask = 0xFF << ((laneId / 8) * 8);
  int rootLane = (laneId / 8) * 8;  // (0, 8, 16, or 24)

  float vara = 0;
  float varb = 0;
  float rho = 0;
  float a = Aa[laneId];
  float b = Bb[laneId];

  float a2 = a*a;
  float b2 = b*b;
  float ab = a*b;
  float a0 = 0.0;
  float b0 = 0.0;

  float value_to_sum_a2 = (typeId == 0) ? 0.0f : a2;
  float value_to_sum_b2 = (typeId == 0) ? 0.0f : b2;
  float value_to_sum_rho = (typeId > 0 && typeId < 7) ? ab : 0.0f;

  // Perform the reduction. The sum lands in the root lane of each segment.
  float sum_1_to_7_a2 = reduce_sum_8_lanes(segmentMask, value_to_sum_a2);
  float sum_1_to_7_b2 = reduce_sum_8_lanes(segmentMask, value_to_sum_b2);
  float sum_1_to_6_rho = reduce_sum_8_lanes(segmentMask, value_to_sum_rho);

  float theta = 0.0f;
  float alpha = 0.0f;
  float ga = 0.0f;
  float Ta = 0.0f;
  // Only the root lane of each segment stores the calculated sum in its 'vara'
  if (laneId == rootLane) { // Equivalent to checking typeId == 0
    vara = sum_1_to_7_a2;
    varb = sum_1_to_7_b2;
    rho = sum_1_to_6_rho;
    float variance_sum = vara + varb - 2.0f * rho;
    if (variance_sum >= 0.0f) {
      theta = sqrtf(variance_sum);
      alpha = (a - b) / theta;
      ga = 0.3989422804f * expf(-(alpha*alpha)/2);
      Ta = (1 + erff(alpha/1.41421356237f))/2;
    }
  }

  theta = __shfl_sync(segmentMask, theta, rootLane);
  ga    = __shfl_sync(segmentMask, ga, rootLane);
  Ta    = __shfl_sync(segmentMask, Ta, rootLane);
  a0    = __shfl_sync(segmentMask, a, rootLane);
  b0    = __shfl_sync(segmentMask, b, rootLane);

  float c = 0.0f;
  float varc = 0.0f;
  if (theta == 0) {
    c = (a0 > b0) ? a : b;
  }
  else {
      if (laneId == rootLane) {
        c = a*Ta + b*(1-Ta) + theta*ga;
        // Clark's E[max^2] - c^2 with the means taken out: computed as written, means of
        // 1 ns (slews) to 40 ns (required times) leave nothing of the variance in float32
        float d = a - b, tg = theta*ga, Tb = 1 - Ta;
        varc = vara*Ta + varb*Tb + d*d*Ta*Tb + d*tg*(Tb - Ta) - tg*tg;
      }
      else {
        c = a*Ta + b*(1-Ta);
      }
  }

  float value_to_sum_c = (typeId > 0 && typeId < 7) ? c*c : 0.0f;
  float sum_1_to_6_c2 = reduce_sum_8_lanes(segmentMask, value_to_sum_c);
  
  Cc[laneId] = c;
  __syncwarp(segmentMask);  // lane root+7 stores its c first, the root lane overwrites it below
  if (laneId == rootLane && theta != 0) {  // theta == 0 copied the larger input, random term too
    //varc -= sum_1_to_7_c2;
    varc -= sum_1_to_6_c2;
    // random term: the variance left after the global sensitivities, 0 if Clark's
    // variance estimate falls below them
    Cc[laneId + 7] = varc > 0 ? sqrtf(varc) : 0.0f;
  }
}


// type = 0 : 0-7  fall early
// type = 1 : 8-15 fall late
// type = 2 : 16-23 rise early
// type = 3 : 24-31 rise late
__global__ void Max(
  const TimingCUDA timing, int num_inputs, int num_gates, int level)
{
  // Shared buffers for merging 32-wide distributions, one set per warp (= per output pin)
  __shared__ float cur_at_sh[BLOCK_MAX], cur_sl_sh[BLOCK_MAX];
  __shared__ float next_at_sh[BLOCK_MAX], next_sl_sh[BLOCK_MAX];

  int gate_idx = blockIdx.x;
  if (gate_idx >= num_gates) return;


  int level_ptr = timing.cell_level_ptr[level];
  int cell_ptr  = level_ptr + gate_idx;
  int has_clk   = timing.cell_hasclk[cell_ptr];

  int ipin_st = timing.ipin_ptr[cell_ptr + num_inputs];
  int ipin_en = timing.ipin_ptr[cell_ptr + num_inputs + 1];
  int numip = ipin_en - ipin_st;

  int opin_st = timing.opin_ptr[cell_ptr];
  int opin_en = timing.opin_ptr[cell_ptr + 1];
  int numop = opin_en - opin_st;
  
  int ipint_st = timing.ipint_ptr[cell_ptr];

  // one warp per opin
  int warp_id = threadIdx.x >> 5;
  if (warp_id >= numop) return;
  int out_p = timing.opin[opin_st + warp_id];

  int lane_id = threadIdx.x & 31;
  int type = lane_id >> 3; // 0–3 for (fallEarly, fallLate, riseEarly, riseLate)
  float* cur_at  = &cur_at_sh[warp_id * 32];
  float* cur_sl  = &cur_sl_sh[warp_id * 32];
  float* next_at = &next_at_sh[warp_id * 32];
  float* next_sl = &next_sl_sh[warp_id * 32];

  // 2) Single-input: direct copy
  if (has_clk || numip <= 1) {
    // there are lots of input not only ck but also d, rst
    int timing_ptr = ipint_st * 32 + (warp_id * numip * 32) + lane_id; 
    float at = timing.ipint_timing_data_at[timing_ptr];
    float sl = timing.ipint_timing_data_slew[timing_ptr];
    timing.opin_timing_data_at [out_p*32 + lane_id] = at;
    timing.opin_timing_data_slew[out_p*32 + lane_id] = sl;
    return;
  }

  // 3) Initialize accumulation with first input pin
  int timing_ptr = ipint_st * 32 + (warp_id * numip * 32) + lane_id;
  float at0 = timing.ipint_timing_data_at[timing_ptr];
  float sl0 = timing.ipint_timing_data_slew[timing_ptr];

  bool isEarly = ((type & 1) == 0);  // early segments require min => use -value
  if (isEarly) { at0 = -at0; sl0 = -sl0; }
  cur_at[lane_id] = at0;
  cur_sl[lane_id] = sl0;

  __syncwarp();


  // 4) Merge each additional input via deterministic max
  for (int j = 1; j < numip; ++j) { 
    timing_ptr = ipint_st * 32 + (warp_id * numip * 32) + j * 32 + lane_id;
    float atj = timing.ipint_timing_data_at[timing_ptr];
    float slj = timing.ipint_timing_data_slew[timing_ptr];

    if (isEarly) { atj = -atj; slj = -slj; }
    next_at[lane_id] = atj;
    next_sl[lane_id] = slj;
    __syncwarp();
    max_kernel(cur_at, cur_at, next_at);
    max_kernel(cur_sl, cur_sl, next_sl);
    __syncwarp();
  }

  // 5) Finalize and write back
  float out_at = cur_at[lane_id];
  float out_sl = cur_sl[lane_id];
  if (isEarly) { out_at = -out_at; out_sl = -out_sl; }
  timing.opin_timing_data_at[out_p*32 + lane_id] = out_at;
  timing.opin_timing_data_slew[out_p*32 + lane_id] = out_sl;
}



// Backward max for rat
__global__ void MaxR(
  const TimingCUDA timing, int num_inputs, int num_gates, int level)
{
  // Shared buffers for merging 32-wide distributions, one set per warp (= per input pin)
  __shared__ float cur_rat_sh[BLOCK_MAXR], next_rat_sh[BLOCK_MAXR];

  int gate_idx = blockIdx.x;
  if (gate_idx >= num_gates) return;

  int cell_ptr = timing.map_to_forward[timing.cell_blevel_ptr[level] + gate_idx];
  int has_clk   = timing.cell_hasclk[cell_ptr];

  int ipin_st = timing.ipin_ptr[cell_ptr + num_inputs];
  int ipin_en = timing.ipin_ptr[cell_ptr + num_inputs + 1];
  int numip = ipin_en - ipin_st;

  int opin_st = timing.opin_ptr[cell_ptr];
  int opin_en = timing.opin_ptr[cell_ptr + 1];
  int numop = opin_en - opin_st;
  int ipint_st = timing.ipint_ptr[cell_ptr];

  // one warp per ipin and iterate all output pins
  int warp_id = threadIdx.x >> 5;
  if (warp_id >= numip) return;
  int in_p  = timing.ipin[ipin_st + warp_id];

  int lane_id = threadIdx.x & 31;
  int type = lane_id >> 3;  // 0–3 for (fallEarly, fallLate, riseEarly, riseLate)
  float* cur_rat  = &cur_rat_sh[warp_id * 32];
  float* next_rat = &next_rat_sh[warp_id * 32];

  // 2) Single output: direct copy
  if (has_clk) return;
  if (numop <= 1) {
    int timing_ptr = ipint_st * 32 + (warp_id * 32) + lane_id;
    float rat = timing.ipint_timing_data_rat[timing_ptr];
    timing.ipin_timing_data_rat[in_p*32 + lane_id] = rat;
    return;
  }

  // 3) Initialize accumulation with first output pin
  int timing_ptr = ipint_st * 32 + (warp_id * 32) + lane_id;
  float rat0 = timing.ipint_timing_data_rat[timing_ptr];

  // required times: early takes the max, late the min (as the wire backward) => late uses -value
  bool isLate = ((type & 1) == 1);
  if (isLate) { rat0 = -rat0; }
  cur_rat[lane_id] = rat0;

  __syncwarp();

  // 4) Merge each additional output via deterministic max
  for (int j = 1; j < numop; ++j) {
    timing_ptr = ipint_st * 32 + (warp_id * 32) + j * numip * 32 + lane_id;
    float ratj = timing.ipint_timing_data_rat[timing_ptr];

    if (isLate) { ratj = -ratj; }
    next_rat[lane_id] = ratj;
    __syncwarp();
    max_kernel(cur_rat, cur_rat, next_rat);
    __syncwarp();
  }

  // 5) Finalize and write back
  float out_rat = cur_rat[lane_id];
  if (isLate) { out_rat = -out_rat; }
  timing.ipin_timing_data_rat[in_p*32 + lane_id] = out_rat;
}



// [ssta-bench-tuning] addWireParamsBackwardMG
template <int GPB>
__global__ void addWireParamsBackwardMG(
  TimingCUDA timing, RctCUDA rct, int num_inputs, int start_idx,
  int num_gates, int level)
{
  __shared__ float cur_rat_sh [GPB * 32];
  __shared__ float next_rat_sh[GPB * 32];

  const int warp_in_block = threadIdx.x >> 5;
  const int gate_idx      = blockIdx.x * GPB + warp_in_block;
  if (gate_idx >= num_gates) return;

  float* cur_rat  = &cur_rat_sh [warp_in_block * 32];
  float* next_rat = &next_rat_sh[warp_in_block * 32];

  const int lane_id = threadIdx.x & 31;
  const int mode    = lane_id >> 3;
  const int sens    = lane_id & 7;

  int pin_idx;
  int port_id;
  int nipin_beg;
  if (level == -1) {
    pin_idx   = gate_idx;
    port_id   = timing.wp[pin_idx];
    nipin_beg = timing.nipin_ptr[pin_idx];
    if (port_id < 0) return;  // no RC net (an unused input)
    int tap_st   = rct.taps_st[port_id];
    int tap_en   = rct.taps_st[port_id + 1];
    int num_taps = tap_en - tap_st;
    bool isLate = ((mode & 1) == 1);
    float rat;
    for (int i = 0; i < num_taps; ++i) {
      int tap = tap_st + i;
      int nipin_idx = timing.nipin[nipin_beg + i];
      rat = timing.ipin_timing_data_rat[nipin_idx * 32 + lane_id];
      if (sens == 0) rat -= rct.tapDelay[4 * tap + (mode / 2)];
      if (sens == 6) rat -= rct.tapDelay[4 * tap + (mode / 2) + 2];
      if (i == 0) { cur_rat[lane_id] = rat; __syncwarp(); }
      else {
        next_rat[lane_id] = rat; __syncwarp();
        if (isLate) { cur_rat[lane_id] = -cur_rat[lane_id]; next_rat[lane_id] = -next_rat[lane_id]; }
        __syncwarp();
        max_kernel(cur_rat, cur_rat, next_rat); __syncwarp();
        if (isLate) cur_rat[lane_id] = -cur_rat[lane_id];
      }
    }
    rat = cur_rat[lane_id];
    timing.ipin_timing_data_rat[gate_idx * 32 + lane_id] = rat;
  } else {
    int cell_ptr = timing.map_to_forward[timing.cell_blevel_ptr[level] + gate_idx];
    int has_clk  = timing.cell_hasclk[cell_ptr];
    if (has_clk) return;
    int opin_st = timing.opin_ptr[cell_ptr];
    int opin_en = timing.opin_ptr[cell_ptr + 1];
    int numop   = opin_en - opin_st;
    int bnipin_ptr_each_gate = timing.bnipin_ptr_each_gate[start_idx + gate_idx];
    for (int i = 0; i < numop; ++i) {
      pin_idx   = timing.opin[opin_st + i];
      port_id   = timing.wp[pin_idx + num_inputs];
      nipin_beg = timing.bnipin_ptr[bnipin_ptr_each_gate + i];
      if (port_id < 0) continue;  // output pin without an RC net
      int tap_st   = rct.taps_st[port_id];
      int tap_en   = rct.taps_st[port_id + 1];
      int num_taps = tap_en - tap_st;
      bool isLate = ((mode & 1) == 1);
      float rat;
      for (int j = 0; j < num_taps; ++j) {
        int tap = tap_st + j;
        int nipin_idx = timing.nipin[nipin_beg + j];
        rat = timing.ipin_timing_data_rat[nipin_idx * 32 + lane_id];
        if (sens == 0) rat -= rct.tapDelay[4 * tap + (mode / 2)];
        if (sens == 6) rat -= rct.tapDelay[4 * tap + (mode / 2) + 2];
        if (j == 0) { cur_rat[lane_id] = rat; __syncwarp(); }
        else {
          next_rat[lane_id] = rat; __syncwarp();
          if (isLate) { cur_rat[lane_id] = -cur_rat[lane_id]; next_rat[lane_id] = -next_rat[lane_id]; }
          __syncwarp();
          max_kernel(cur_rat, cur_rat, next_rat); __syncwarp();
          if (isLate) cur_rat[lane_id] = -cur_rat[lane_id];
        }
      }
      rat = cur_rat[lane_id];
      timing.opin_timing_data_rat[pin_idx * 32 + lane_id] = rat;
    }
  }
}
template __global__ void addWireParamsBackwardMG<2> (TimingCUDA, RctCUDA, int, int, int, int);
template __global__ void addWireParamsBackwardMG<4> (TimingCUDA, RctCUDA, int, int, int, int);
template __global__ void addWireParamsBackwardMG<8> (TimingCUDA, RctCUDA, int, int, int, int);
template __global__ void addWireParamsBackwardMG<16>(TimingCUDA, RctCUDA, int, int, int, int);


void _addWireParamsForward(TimingCUDA& timing_cuda, RctCUDA& rct, int num_inputs, int start_idx, int num_pins, int level) {
  constexpr int num_threads = BLOCK_FWD;
  int warp_per_block = num_threads / 32;
  int num_blocks = (num_pins + warp_per_block - 1) / warp_per_block;
  addWireParamsForward<<<num_blocks, num_threads>>>(timing_cuda, rct, num_inputs, start_idx, num_pins, level);
  CUDA_CHECK(cudaGetLastError());
}


void _addWireParamsBackward(TimingCUDA& timing_cuda, RctCUDA& rct, int num_inputs, int start_idx, int num_gates, int level) {
#if GATES_PER_BLOCK_BWD == 1
  int num_blocks = num_gates;
  addWireParamsBackward<<<num_blocks, /*threads_per_block=*/32>>>(
      timing_cuda, rct, num_inputs, start_idx, num_gates, level);
#else
  constexpr int gpb = GATES_PER_BLOCK_BWD;
  constexpr int num_threads = gpb * 32;
  int num_blocks = (num_gates + gpb - 1) / gpb;
  addWireParamsBackwardMG<gpb><<<num_blocks, num_threads>>>(
      timing_cuda, rct, num_inputs, start_idx, num_gates, level);
#endif
  CUDA_CHECK(cudaGetLastError());
}

void _getTDataSense(TimingCUDA& timing_cuda, int num_inputs, int num_gates, int level, int dir) {
  constexpr int num_threads = BLOCK_TDATA;
  int num_blocks = num_gates;
  getTDataSense<<<num_blocks, num_threads>>>(timing_cuda, num_inputs, num_gates, level, dir);
  CUDA_CHECK(cudaGetLastError());
}

void _addDelayParam(TimingCUDA& timing_cuda, RctCUDA& rct, int num_inputs, int num_gates, int level, int dir, float clockTP) {
  constexpr int num_threads = BLOCK_DELAY;
  int num_blocks = num_gates;
  addDelayParam<<<num_blocks, num_threads>>>(timing_cuda, rct, num_inputs, num_gates, level, dir, clockTP);
  CUDA_CHECK(cudaGetLastError());
}

void _max(TimingCUDA& timing_cuda, int num_inputs, int num_gates, int level) {
  constexpr int num_threads = BLOCK_MAX;
  int num_blocks = num_gates;
  Max<<<num_blocks, num_threads>>>(timing_cuda, num_inputs, num_gates, level);
  CUDA_CHECK(cudaGetLastError());
}

void _maxR(TimingCUDA& timing_cuda, int num_inputs, int num_gates, int level) {
  constexpr int num_threads = BLOCK_MAXR;
  int num_blocks = num_gates;
  MaxR<<<num_blocks, num_threads>>>(timing_cuda, num_inputs, num_gates, level);
  CUDA_CHECK(cudaGetLastError());
}
