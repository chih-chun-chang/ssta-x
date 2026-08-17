// =============================================================================
// tuning_macros.cuh -- compile-time tuning surface for the SSTA CUDA kernels.
//
// Apply via scripts/apply_patch.py, which copies this file into your ssta
// include tree and rewrites timing.cu to consume the macros.
//
// Every macro has a default that reproduces the original hand-picked values.
// Override any macro with -DBLOCK_FWD=128 etc. on the nvcc command line
// (handled by tune.py through CMake's TUNING_DEFS cache var).
// =============================================================================

#pragma once

// ---- Per-kernel block sizes (threads per block) ----------------------------
#ifndef BLOCK_FWD
#define BLOCK_FWD 512        // addWireParamsForward
#endif

#ifndef BLOCK_BWD
#define BLOCK_BWD 32         // addWireParamsBackward (single-gate variant)
#endif

#ifndef BLOCK_TDATA
#define BLOCK_TDATA 256      // getTDataSense -- must be <= 512 (smem holds 16 warps)
#endif

#ifndef BLOCK_DELAY
#define BLOCK_DELAY 256      // addDelayParam
#endif

#ifndef BLOCK_MAX
#define BLOCK_MAX 256        // Max  (forward smax)
#endif

#ifndef BLOCK_MAXR
#define BLOCK_MAXR 256       // MaxR (backward smax)
#endif

// ---- Structural knob: gates packed per block in the backward wire kernel ---
// 1 = original behaviour (1 warp per block). >1 selects the multi-gate kernel.
#ifndef GATES_PER_BLOCK_BWD
#define GATES_PER_BLOCK_BWD 1
#endif

// ---- __launch_bounds__ controls (set MAXT > 0 to enable on a kernel) -------
#ifndef LB_FWD_MAXT
#define LB_FWD_MAXT 0
#endif
#ifndef LB_FWD_MINB
#define LB_FWD_MINB 0
#endif

#ifndef LB_DELAY_MAXT
#define LB_DELAY_MAXT 0
#endif
#ifndef LB_DELAY_MINB
#define LB_DELAY_MINB 0
#endif

#ifndef LB_TDATA_MAXT
#define LB_TDATA_MAXT 0
#endif
#ifndef LB_TDATA_MINB
#define LB_TDATA_MINB 0
#endif

#if LB_FWD_MAXT > 0
#  define KERNEL_LB_FWD __launch_bounds__(LB_FWD_MAXT, LB_FWD_MINB)
#else
#  define KERNEL_LB_FWD
#endif

#if LB_DELAY_MAXT > 0
#  define KERNEL_LB_DELAY __launch_bounds__(LB_DELAY_MAXT, LB_DELAY_MINB)
#else
#  define KERNEL_LB_DELAY
#endif

#if LB_TDATA_MAXT > 0
#  define KERNEL_LB_TDATA __launch_bounds__(LB_TDATA_MAXT, LB_TDATA_MINB)
#else
#  define KERNEL_LB_TDATA
#endif

// ---- Sanity checks ---------------------------------------------------------
static_assert(BLOCK_FWD   >= 32 && BLOCK_FWD   <= 1024 && (BLOCK_FWD   % 32) == 0,
              "BLOCK_FWD must be a multiple of 32 in [32, 1024]");
static_assert(BLOCK_BWD   >= 32 && BLOCK_BWD   <= 1024 && (BLOCK_BWD   % 32) == 0,
              "BLOCK_BWD must be a multiple of 32 in [32, 1024]");
static_assert(BLOCK_TDATA >= 32 && BLOCK_TDATA <= 512  && (BLOCK_TDATA % 32) == 0,
              "BLOCK_TDATA must be in [32, 512] (smem allocates 16 warps)");
static_assert(BLOCK_DELAY >= 32 && BLOCK_DELAY <= 1024 && (BLOCK_DELAY % 32) == 0,
              "BLOCK_DELAY must be a multiple of 32 in [32, 1024]");
static_assert(BLOCK_MAX   >= 32 && BLOCK_MAX   <= 1024 && (BLOCK_MAX   % 32) == 0,
              "BLOCK_MAX must be a multiple of 32 in [32, 1024]");
static_assert(BLOCK_MAXR  >= 32 && BLOCK_MAXR  <= 1024 && (BLOCK_MAXR  % 32) == 0,
              "BLOCK_MAXR must be a multiple of 32 in [32, 1024]");
static_assert(GATES_PER_BLOCK_BWD >= 1 && GATES_PER_BLOCK_BWD <= 16,
              "GATES_PER_BLOCK_BWD must be in [1, 16]");
static_assert(GATES_PER_BLOCK_BWD * 32 <= 1024,
              "GATES_PER_BLOCK_BWD * 32 must be <= 1024 threads per block");
