#include <ssta/cuda/rct.cuh>
#include <ssta/cuda/sort.cuh>
#include <ssta/cuda/utils.cuh>

template <int BlockDim>
__global__ void bfs(RctCUDA rct) {
  const int tid = threadIdx.x; 

  __shared__ int num_nodes;
  __shared__ int num_edges;
  __shared__ int nodes_offset; 
  __shared__ int edges_offset;
  __shared__ int* edges_s;
  __shared__ int* edges_t;
  __shared__ float* ress;
  __shared__ float* pres;
  __shared__ int* distances;
  __shared__ int* counts;
  __shared__ int* orders;
  __shared__ int* outputs;
  __shared__ int* pid;
  __shared__ int root;
  __shared__ int level;
  __shared__ int count;
  
  for (int net_id = blockIdx.x; net_id < rct.num_nets; net_id += gridDim.x) {
    if (tid == 0) {
      nodes_offset = rct.nodes_st[net_id]; 
      edges_offset = rct.edges_st[net_id];
      num_nodes    = rct.nodes_st[net_id + 1] - nodes_offset; 
      num_edges    = rct.edges_st[net_id + 1] - edges_offset;
      edges_s      = rct.edges_s + edges_offset; 
      edges_t      = rct.edges_t + edges_offset;
      ress         = rct.ress + edges_offset;
      pres         = rct.pres + nodes_offset; 
      distances    = rct.distances + nodes_offset; 
      counts       = rct.counts + nodes_offset; 
      orders       = rct.orders + nodes_offset; 
      outputs      = rct.outputs + nodes_offset; 
      pid          = rct.pid + nodes_offset; 
      root         = rct.ports[net_id]; 
      level = 0; 
      count = 1;
      pid[root] = -1;
    }

    __syncthreads();

    // initialize distance and sort_counts for root 
    for (int u = tid; u < num_nodes; u += blockDim.x) {
      distances[u] = (u == root)? 0 : INT_MAX; 
      counts[u] = (u == 0)? 1 : 0;
    }

    for (int d = 0; d < num_nodes; ++d) {
      if (tid == 0) count = 0; 
      __syncthreads(); 
      for (int e = tid; e < num_edges; e += blockDim.x) {
        int& es = edges_s[e];
        int& et = edges_t[e];
        int& ds = distances[es]; 
        int& dt = distances[et]; 
        if (ds == level) {
          if (level + 1 < dt) {
            dt = level + 1; 
            atomicAdd(&count, 1);
          }
        }
        else if (dt == level) {
          if (level + 1 < ds) {
            ds = level + 1; 
            atomicAdd(&count, 1);     
            int tmp = es; // swap s -> t to t -> s 
            es = et; 
            et = tmp; 
          }
        }
      }
      __syncthreads();
      if (count) {
        if (tid == 0) {
          level += 1; 
          counts[level] = count;
        }
      }
      else {
        break; 
      }
      __syncthreads();
    }
    __syncthreads(); 
    block_counting_sort<BlockDim>(distances, counts, orders, outputs, num_nodes, 0, num_edges, false); 
    
    for (int e = tid; e < num_edges; e += blockDim.x) {
      const int es = edges_s[e];
      const int et = edges_t[e];
      const float res = ress[e];
      const int ds = distances[es];
      const int dt = distances[et];
      if (ds > dt) {
        pid[es] = et;
        pres[es] = res;
      }
      else {
        pid[et] = es;
        pres[et] = res;
      }
    }
  }
}

// a net a thread
__global__ void compute_rc_timing(RctCUDA rct, float Mc, float Mr) {
  unsigned int net_id = blockIdx.x * blockDim.x + threadIdx.x;
  unsigned int type_idx = threadIdx.y;
  if(net_id >= rct.num_nets) return;
  int nodes_st = rct.nodes_st[net_id]; 
  int nodes_en = rct.nodes_st[net_id + 1];
  int taps_st = rct.taps_st[net_id];
  int taps_en = rct.taps_st[net_id + 1];

  // init
  for (int i = nodes_st; i < nodes_en; ++i) {
    
    float cap = rct.caps[i];
    float Mccaps = Mc*cap;
    
    if (type_idx < 2) {
      rct.load[4*i + type_idx] = cap;
      rct.beta[4*i + type_idx] = cap;
    }
    else {
      rct.load[4*i + type_idx] = Mccaps;
      rct.beta[4*i + type_idx] = Mccaps;
    }
  }

  // ipin cap
  for (int i = taps_st; i < taps_en; ++i) {
    int tap = rct.taps[i];
    int node = nodes_st + tap;
    rct.load[4*node + type_idx] = rct.tapCaps[2*i + type_idx%2];
  }
  
  // downstream load (nodes_st is port node and its prev = -1)
  for (int i = nodes_en - 1; i > nodes_st; --i) {
    int n = rct.orders[i] + nodes_st;
    int prev = rct.pid[n] + nodes_st;
    rct.load[4*prev + type_idx] += rct.load[4*n + type_idx];
  }

  // upstream delay
  for (int i = nodes_st+1; i < nodes_en; ++i) {
    int n = rct.orders[i] + nodes_st;
    int prev = rct.pid[n] + nodes_st;
    float pres = rct.pres[n];
    if (type_idx < 2) {
      rct.delay[4*n + type_idx] += rct.delay[4*prev + type_idx] + pres*rct.load[4*n + type_idx];
    }
    else {
      rct.delay[4*n + type_idx] += rct.delay[4*prev + type_idx] + Mr*pres*rct.load[4*n + type_idx];
    }
  }
  
  // add tap's ipin cap
  for (int i = taps_st; i < taps_en; ++i) {
    int tap = rct.taps[i];
    int node = nodes_st + tap;
    rct.beta[4*node + type_idx] += rct.tapCaps[2*i + type_idx%2];
  }

  // multiply delay
  for (int i = nodes_st; i < nodes_en; ++i) {
    rct.beta[4*i + type_idx] *= rct.delay[4*i + type_idx];
  }

  // downstream beta (nodes_st is port node and its prev = -1)
  for (int i = nodes_en - 1; i > nodes_st; --i) {
    int n = rct.orders[i] + nodes_st;
    int prev = rct.pid[n] + nodes_st;
    rct.beta[4*prev + type_idx] += rct.beta[4*n + type_idx];
  }

  // upstream beta
  for (int i = nodes_st+1; i < nodes_en; ++i) {
    int n = rct.orders[i] + nodes_st;
    int prev = rct.pid[n] + nodes_st;
    float pres = rct.pres[n];
    if (type_idx < 2) {
      rct.beta[4*n + type_idx] += rct.beta[4*prev + type_idx] + pres*rct.beta[4*n + type_idx];
    }
    else
    {
      rct.beta[4*n + type_idx] += rct.beta[4*prev + type_idx] + Mr*pres*rct.beta[4*n + type_idx];
    }
  }

  // calculate slew
  for (int i = nodes_st; i < nodes_en; ++i) {
    float d = rct.delay[4*i + type_idx];
    rct.slew[4*i + type_idx] = sqrtf(2*rct.beta[4*i + type_idx]-d*d);
  }

  // for each tap
  for (int i = taps_st; i < taps_en; ++i) {
    int tap = rct.taps[i] + nodes_st;
    
    if (type_idx < 2) {
      rct.tapDelay[4*i + type_idx] = rct.delay[4*tap + type_idx];
      rct.tapSlew[4*i + type_idx] = rct.slew[4*tap + type_idx];
    }
    else {
      rct.tapDelay[4*i + type_idx] = (rct.delay[4*tap + type_idx] - rct.delay[4*tap + type_idx - 2])/3;
      rct.tapSlew[4*i + type_idx] = (rct.slew[4*tap + type_idx] - rct.slew[4*tap + type_idx - 2])/3;
    }
  }
  

}

// a net a block
// num_threads = max(num_nodes) = blockDim.x
// shared memory size = num_nodes
__global__ void compute_rc_timing_block(RctCUDA rct, float Mc, float Mr, int numElements) {
  const int tid = threadIdx.x; 

  __shared__ int nodes_st;
  __shared__ int nodes_en;
  __shared__ int taps_st;
  __shared__ int taps_en;
  extern __shared__ char smem[];

  // Define pointers to each array with proper offsets
  int* staps = (int*)smem;
  float* scaps = (float*)(staps + numElements);
  float* stcaps_r = scaps + numElements;
  float* stcaps_f = stcaps_r + numElements;
  float* lnr = stcaps_f + numElements;
  float* lnf = lnr + numElements;
  float* lsr = lnf + numElements;
  float* lsf = lsr + numElements;
  float* bnr = lsf + numElements;
  float* bnf = bnr + numElements;
  float* bsr = bnf + numElements;
  float* bsf = bsr + numElements;
  float* dnr = bsf + numElements;
  float* dnf = dnr + numElements;
  float* dsr = dnf + numElements;
  float* dsf = dsr + numElements;
  float* snr = dsf + numElements;
  float* snf = snr + numElements;
  float* ssr = snf + numElements;
  float* ssf = ssr + numElements;

  for (int net_id = blockIdx.x; net_id < rct.num_nets; net_id += gridDim.x) {
    if (tid == 0) {
      nodes_st = rct.nodes_st[net_id];
      nodes_en = rct.nodes_st[net_id+1];
      taps_st = rct.taps_st[net_id];
      taps_en = rct.taps_st[net_id + 1];
    }
    scaps[tid] = 0;
    stcaps_r[tid] = 0;
    stcaps_f[tid] = 0;
    __syncthreads();
    
    int ns = nodes_st; // broadcast
    int ne = nodes_en;
    int ts = taps_st;
    int te = taps_en;
    int num_nodes = ne - ns;
    int num_taps = te - ts; // num_nodes - 1
    int order;

    if (tid < num_nodes) {
      scaps[tid] = rct.caps[ns+tid];   
      order = rct.orders[ns+tid];
      float cap = scaps[tid];
      float Mccaps = Mc*cap;
      lnr[tid] = cap;
      lnf[tid] = cap;
      lsr[tid] = Mccaps;
      lsf[tid] = Mccaps;
      bnr[tid] = cap;
      bnf[tid] = cap;
      bsr[tid] = Mccaps;
      bsf[tid] = Mccaps;
    }

    if (tid < num_taps) {
      staps[tid] = rct.taps[ts+tid];
      stcaps_r[tid] = rct.tapCaps[ts+tid];
      stcaps_f[tid] = rct.tapCaps[ts+tid+1];
      int tap = staps[tid];
      float tcapr = stcaps_r[tid]; 
      float tcapf = stcaps_f[tid];
      lnr[tap] += tcapr;
      lnf[tap] += tcapf;
      lsr[tap] += tcapr;
      lsf[tap] += tcapf;
      bnr[tap] += tcapr;
      bnf[tap] += tcapf;
      bsr[tap] += tcapr;
      bsf[tap] += tcapf;
    } 

    int n = order + ns;
    int prev = rct.pid[n]; 
    float pres = rct.pres[n];
    if (tid < num_nodes) {
      // downstream
      for (int currentOrder = num_nodes-1; currentOrder > 0; currentOrder--) {
        __syncthreads();
        if (tid == currentOrder) { 
          lnr[prev] += lnr[order];
          lnf[prev] += lnf[order];
          lsr[prev] += lsr[order];
          lsf[prev] += lsf[order];
        }
      }
      // upstream
      if (tid == 0) {
        dnr[tid] = pres*lnr[tid];
        dnf[tid] = pres*lnf[tid];
        dsr[tid] = Mr*pres*lsr[tid];
        dsf[tid] = Mr*pres*lsf[tid];
      }
      
      for (int currentOrder = 1; currentOrder < num_nodes; currentOrder++) {
        __syncthreads();
        if (tid == currentOrder) {
          dnr[order] += dnr[prev] + pres*lnr[order];
          dnf[order] += dnf[prev] + pres*lnf[order];
          dsr[order] += dsr[prev] + Mr*pres*lsr[order];
          dsf[order] += dsf[prev] + Mr*pres*lsf[order];
        }
      }
      bnr[tid] *= dnr[tid];
      bnf[tid] *= dnf[tid];
      bsr[tid] *= dsr[tid];
      bsf[tid] *= dsf[tid];
      
      // downstream
      for (int currentOrder = num_nodes-1; currentOrder > 0; currentOrder--) {
        __syncthreads();
        if (tid == currentOrder) { 
          bnr[prev] += bnr[order];
          bnf[prev] += bnf[order];
          bsr[prev] += bsr[order];
          bsf[prev] += bsf[order];
        }
        //__syncthreads();
      }
      // upstream
      for (int currentOrder = 1; currentOrder < num_nodes; currentOrder++) {
        __syncthreads();
        if (tid == currentOrder) { 
          bnr[order] += bnr[prev] + pres*bnr[order];
          bnf[order] += bnf[prev] + pres*bnf[order];
          bsr[order] += bsr[prev] + Mr*pres*bsr[order];
          bsf[order] += bsf[prev] + Mr*pres*bsf[order];
        }
        //__syncthreads();
      }
      float _dnr = dnr[tid];
      float _dnf = dnf[tid];
      float _dsr = dsr[tid];
      float _dsf = dsf[tid];
      snr[tid] = sqrtf(2*bnr[tid] - _dnr*_dnr);
      snf[tid] = sqrtf(2*bnf[tid] - _dnf*_dnf);
      ssr[tid] = sqrtf(2*bsr[tid] - _dsr*_dsr);
      ssf[tid] = sqrtf(2*bsf[tid] - _dsf*_dsf);
    }

    // write result
    if (tid < num_taps) {
      int tap = staps[tid];
      rct.tapDelay[ts+tid] = dnr[tap];
      rct.tapDelay[ts+tid+1] = dnf[tap];
      rct.tapDelay[ts+tid+2] = (dsr[tap]-dnr[tap])/3;
      rct.tapDelay[ts+tid+3] = (dsf[tap]-dnf[tap])/3;
      rct.tapSlew[ts+tid] = snr[tap];
      rct.tapSlew[ts+tid+1] = snf[tap];
      rct.tapSlew[ts+tid+2] = (ssr[tap]-snr[tap])/3;
      rct.tapSlew[ts+tid+3] = (ssf[tap]-snf[tap])/3;
    }
  }
}

__global__ void compute_rc_timing2(RctCUDA rct, float Mc, float Mr) {
  unsigned int net_id = blockIdx.x * blockDim.x + threadIdx.x;
  unsigned int type_idx = threadIdx.y;
  // no early return: every thread must reach the __syncthreads() below
  const bool active = net_id < rct.num_nets;
  int nodes_st = 0, nodes_en = 0, taps_st = 0, taps_en = 0;
  float outCap = 0.0f;

  // type id
  // 0,1 = nominal (f, r)
  // 2,3 = sense (f, r)
  if (active) {
    nodes_st = rct.nodes_st[net_id];
    nodes_en = rct.nodes_st[net_id + 1];
    taps_st = rct.taps_st[net_id];
    taps_en = rct.taps_st[net_id + 1];

    float mc = 1.0f;
    float mr = 1.0f;
    if (type_idx > 1) {
      mc = Mc;
      mr = Mr;
    }

    for (int i = nodes_st; i < nodes_en; ++i) {
      float cap = mc*rct.caps[i];
      rct.load[4*i + type_idx] = cap;
      rct.beta[4*i + type_idx] = cap;
      outCap += cap;
    }

    // ipin cap
    for (int i = taps_st; i < taps_en; ++i) {
      int tap = rct.taps[i] + nodes_st;
      float tapCap = rct.tapCaps[2*i + type_idx%2];
      rct.load[4*tap + type_idx] += tapCap;
      rct.beta[4*tap + type_idx] += tapCap;
      outCap += tapCap;
    }

    // downstream load (nodes_st is port node and its prev = -1)
    for (int i = nodes_en - 1; i > nodes_st; --i) {
      int n = rct.orders[i] + nodes_st;
      int prev = rct.pid[n] + nodes_st;
      rct.load[4*prev + type_idx] += rct.load[4*n + type_idx];
    }

    // the root (port) starts the delay and beta accumulation at zero
    if (nodes_en > nodes_st) {
      int root = rct.orders[nodes_st] + nodes_st;
      rct.delay[4*root + type_idx] = 0.0f;
      rct.slew[4*root + type_idx] = 0.0f;
    }

    // upstream delay
    for (int i = nodes_st+1; i < nodes_en; ++i) {
      int n = rct.orders[i] + nodes_st;
      int prev = rct.pid[n] + nodes_st;
      float pres = rct.pres[n];
      rct.delay[4*n + type_idx] = rct.delay[4*prev + type_idx] + mr*pres*rct.load[4*n + type_idx];
    }

    // B
    // multiply delay
    for (int i = nodes_st; i < nodes_en; ++i) {
      rct.beta[4*i + type_idx] *= rct.delay[4*i + type_idx];
    }

    // downstream beta (nodes_st is port node and its prev = -1)
    for (int i = nodes_en - 1; i > nodes_st; --i) {
      int n = rct.orders[i] + nodes_st;
      int prev = rct.pid[n] + nodes_st;
      rct.beta[4*prev + type_idx] += rct.beta[4*n + type_idx];
    }

    // upstream beta
    for (int i = nodes_st+1; i < nodes_en; ++i) {
      int n = rct.orders[i] + nodes_st;
      int prev = rct.pid[n] + nodes_st;
      float pres = rct.pres[n];
      rct.slew[4*n + type_idx] = rct.slew[4*prev + type_idx] + mr*pres*rct.beta[4*n + type_idx];
    }

    // for each tap
    for (int i = taps_st; i < taps_en; ++i) {
      int tap = rct.taps[i] + nodes_st;
      float b = rct.slew[4*tap + type_idx];
      float d = rct.delay[4*tap + type_idx];
      rct.tapDelay[4*i + type_idx] = d;
      rct.tapSlew[4*i + type_idx] = sqrtf(2*b - d*d);
    }

    rct.outCap[4*net_id + type_idx] = outCap;
  }

  // the sensitivity rows (type 2,3) read the nominal rows (type 0,1) written by other warps
  __syncthreads();

  if (active && type_idx > 1) {
    for (int i = taps_st; i < taps_en; ++i) {
      int tap = rct.taps[i] + nodes_st;
      float bn = rct.slew[4*tap + type_idx%2];
      float dn = rct.delay[4*tap + type_idx%2];
      rct.tapDelay[4*i + type_idx] = (rct.tapDelay[4*i + type_idx] - dn)/3;
      rct.tapSlew[4*i + type_idx] = (rct.tapSlew[4*i + type_idx] - sqrtf(2*bn - dn*dn))/3;
    }
    rct.outCap[4*net_id + type_idx] = (outCap - rct.outCap[4*net_id + type_idx%2])/3;
  }
}
// -------------------------------------------------------------------------

void _bfs(RctCUDA& rct_cuda) {
  constexpr int num_threads = 128;
  bfs<num_threads><<<rct_cuda.num_nets, num_threads>>>(rct_cuda);
  CUDA_CHECK(cudaGetLastError());
}

void _compute_rc_timing(RctCUDA& rct_cuda, float Mc, float Mr) {
  constexpr int num_threads = 128;
  dim3 threadsPerBlock(num_threads, 4);
  dim3 blocksPerGrid((rct_cuda.num_nets + threadsPerBlock.x - 1) / threadsPerBlock.x, 1);
  compute_rc_timing<<<blocksPerGrid, threadsPerBlock>>>(rct_cuda, Mc, Mr);
  CUDA_CHECK(cudaGetLastError());
}

void _compute_rc_timing2(RctCUDA& rct_cuda, float Mc, float Mr) {
  constexpr int num_threads = 128;
  dim3 threadsPerBlock(num_threads, 4);
  dim3 blocksPerGrid((rct_cuda.num_nets + threadsPerBlock.x - 1) / threadsPerBlock.x, 1);
  compute_rc_timing2<<<blocksPerGrid, threadsPerBlock>>>(rct_cuda, Mc, Mr);
  CUDA_CHECK(cudaGetLastError());
}

void _compute_rc_timing_block(RctCUDA& rct_cuda, float Mc, float Mr) {
  // num threads = num_nodes
  constexpr int num_threads = 128;
  int numTaps = 128;
  int numElements = 128;
  size_t sharedMemSize = (numTaps * sizeof(int)) + (19 * numElements * sizeof(float));
  compute_rc_timing_block<<<rct_cuda.num_nets, num_threads, sharedMemSize>>>(rct_cuda, Mc, Mr, 128);
  CUDA_CHECK(cudaGetLastError());
}
