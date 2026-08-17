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
  //__shared__ int* nodes_st;
  //__shared__ int* edges_st;
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
  
  // TODO: make the edges in shared mem (PTX)
  //extern __shared__ float edges_s[];
  //extern __shared__ float edges_t[];

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
    // argsort nodes according to distances to root 
    // TODO: blocksort
    block_counting_sort<BlockDim>(distances, counts, orders, outputs, num_nodes, 0, num_edges, false); 
    // construct pid
    // TODO: potential issue
    //__syncthreads(); 
    
    for (int e = tid; e < num_edges; e += blockDim.x) {
      const int es = edges_s[e];
      const int et = edges_t[e];
      const float res = ress[e];
      const int ds = distances[es];
      const int dt = distances[et];
      // race condition!?
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
  //float tapCaps_r_overall = 0;
  //float tapCaps_f_overall = 0;
  //float caps_overall = 0;
  //float Mccaps_overall = 0;

  // init
  for (int i = nodes_st; i < nodes_en; ++i) {
    
    float cap = rct.caps[i];
    float Mccaps = Mc*cap;
    /*
    rct.load_n_r[i] = cap;
    rct.load_n_f[i] = cap;
    rct.load_s_r[i] = Mccaps;
    rct.load_s_f[i] = Mccaps;
    rct.beta_n_r[i] = cap;
    rct.beta_n_f[i] = cap;
    rct.beta_s_r[i] = Mccaps;
    rct.beta_s_f[i] = Mccaps;
    caps_overall += cap;
    Mccaps_overall += Mccaps;
    */
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
    /*
    float tapCaps_r = rct.tapCaps_r[i];
    float tapCaps_f = rct.tapCaps_f[i];
    rct.load_n_r[node] += tapCaps_r;
    rct.load_n_f[node] += tapCaps_f;
    rct.load_s_r[node] += tapCaps_r;
    rct.load_s_f[node] += tapCaps_f;
    tapCaps_r_overall += tapCaps_r;
    tapCaps_f_overall += tapCaps_f;
    */
    rct.load[4*node + type_idx] = rct.tapCaps[2*i + type_idx%2];
  }
  
  // downstream load (nodes_st is port node and its prev = -1)
  for (int i = nodes_en - 1; i > nodes_st; --i) {
    int n = rct.orders[i] + nodes_st;
    int prev = rct.pid[n] + nodes_st;
    /*
    rct.load_n_r[prev] += rct.load_n_r[n];
    rct.load_n_f[prev] += rct.load_n_f[n];
    rct.load_s_r[prev] += rct.load_s_r[n];
    rct.load_s_f[prev] += rct.load_s_f[n];
    */
    rct.load[4*prev + type_idx] += rct.load[4*n + type_idx];
  }

  // upstream delay
  for (int i = nodes_st+1; i < nodes_en; ++i) {
    int n = rct.orders[i] + nodes_st;
    int prev = rct.pid[n] + nodes_st;
    float pres = rct.pres[n];
    /*
    if (prev == -1) {
      rct.delay_n_r[n] = pres*rct.load_n_r[n];
      rct.delay_n_f[n] = pres*rct.load_n_f[n];
      rct.delay_s_r[n] = Mr*pres*rct.load_s_r[n];
      rct.delay_s_f[n] = Mr*pres*rct.load_s_f[n];
    }
    else {
      rct.delay_n_r[n] += rct.delay_n_r[prev] + pres*rct.load_n_r[n];
      rct.delay_n_f[n] += rct.delay_n_f[prev] + pres*rct.load_n_f[n];
      rct.delay_s_r[n] += rct.delay_s_r[prev] + Mr*pres*rct.load_s_r[n];
      rct.delay_s_f[n] += rct.delay_s_f[prev] + Mr*pres*rct.load_s_f[n];
    }
    */
    if (type_idx < 2) {
      rct.delay[4*n + type_idx] += rct.delay[4*prev + type_idx] + pres*rct.load[4*n + type_idx];
    }
    else {
      rct.delay[4*n + type_idx] += rct.delay[4*prev + type_idx] + Mr*pres*rct.load[4*n + type_idx];
    }
  }
  
  // B
  // add tap's ipin cap
  for (int i = taps_st; i < taps_en; ++i) {
    int tap = rct.taps[i];
    int node = nodes_st + tap;
    /*
    rct.beta_n_r[node] += rct.tapCaps_r[i];
    rct.beta_n_f[node] += rct.tapCaps_f[i];
    rct.beta_s_r[node] += rct.tapCaps_r[i];
    rct.beta_s_f[node] += rct.tapCaps_f[i];
    */
    rct.beta[4*node + type_idx] += rct.tapCaps[2*i + type_idx%2];
  }

  // multiply delay
  for (int i = nodes_st; i < nodes_en; ++i) {
    /*
    rct.beta_n_r[i] *= rct.delay_n_r[i];
    rct.beta_n_f[i] *= rct.delay_n_f[i];
    rct.beta_s_r[i] *= rct.delay_s_r[i];
    rct.beta_s_f[i] *= rct.delay_s_f[i];
    */
    rct.beta[4*i + type_idx] *= rct.delay[4*i + type_idx];
  }

  // downstream beta (nodes_st is port node and its prev = -1)
  for (int i = nodes_en - 1; i > nodes_st; --i) {
    int n = rct.orders[i] + nodes_st;
    int prev = rct.pid[n] + nodes_st;
    /*
    rct.beta_n_r[prev] += rct.beta_n_r[n];
    rct.beta_n_f[prev] += rct.beta_n_f[n];
    rct.beta_s_r[prev] += rct.beta_s_r[n];
    rct.beta_s_f[prev] += rct.beta_s_f[n];
    */
    rct.beta[4*prev + type_idx] += rct.beta[4*n + type_idx];
  }

  // upstream beta
  for (int i = nodes_st+1; i < nodes_en; ++i) {
    int n = rct.orders[i] + nodes_st;
    int prev = rct.pid[n] + nodes_st;
    float pres = rct.pres[n];
    /*
    rct.beta_n_r[n] += rct.beta_n_r[prev] + pres*rct.beta_n_r[n];
    rct.beta_n_f[n] += rct.beta_n_f[prev] + pres*rct.beta_n_f[n];
    rct.beta_s_r[n] += rct.beta_s_r[prev] + Mr*pres*rct.beta_s_r[n];
    rct.beta_s_f[n] += rct.beta_s_f[prev] + Mr*pres*rct.beta_s_f[n];
    */
    // TODO: remove if by enlarge the pres array
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
    /*
    float dnr = rct.delay_n_r[i];
    float dnf = rct.delay_n_f[i];
    float dsr = rct.delay_s_r[i];
    float dsf = rct.delay_s_f[i];
    rct.slew_n_r[i] = sqrtf(2*rct.beta_n_r[i]-(dnr*dnr)); //__fsqrt_rn(x)
    rct.slew_n_f[i] = sqrtf(2*rct.beta_n_f[i]-(dnf*dnf));
    rct.slew_s_r[i] = sqrtf(2*rct.beta_s_r[i]-(dsr*dsr));
    rct.slew_s_f[i] = sqrtf(2*rct.beta_s_f[i]-(dsf*dsf));
    */
    float d = rct.delay[4*i + type_idx];
    rct.slew[4*i + type_idx] = sqrtf(2*rct.beta[4*i + type_idx]-d*d);
  }

  // for each tap
  for (int i = taps_st; i < taps_en; ++i) {
    int tap = rct.taps[i] + nodes_st;
    
    //float dnr = rct.delay_n_r[tap];
    //float dnf = rct.delay_n_f[tap];
    //float snr = rct.slew_n_r[tap];
    //float snf = rct.slew_n_f[tap];;
    //rct.tapDelay_n_r[i] = dnr; // this is nominal
    //rct.tapDelay_n_f[i] = dnf;
    //rct.tapDelay_s_r[i] = (rct.delay_s_r[tap] - dnr)/3; // this is sensitivity
    //rct.tapDelay_s_f[i] = (rct.delay_s_f[tap] - dnf)/3;
    //rct.tapSlew_n_r[i] = snr;
    //rct.tapSlew_n_f[i] = snf;
    //rct.tapSlew_s_r[i] = (rct.slew_s_r[tap] - snr)/3;
    //rct.tapSlew_s_f[i] = (rct.slew_s_f[tap] - snf)/3;
    
    if (type_idx < 2) {
      rct.tapDelay[4*i + type_idx] = rct.delay[4*tap + type_idx];
      rct.tapSlew[4*i + type_idx] = rct.slew[4*tap + type_idx];
    }
    else {
      rct.tapDelay[4*i + type_idx] = (rct.delay[4*tap + type_idx] - rct.delay[4*tap + type_idx - 2])/3;
      rct.tapSlew[4*i + type_idx] = (rct.slew[4*tap + type_idx] - rct.slew[4*tap + type_idx - 2])/3;
    }
  }
  

  // out cap
  /*
  float rtc = tapCaps_r_overall + caps_overall;
  float ftc = tapCaps_f_overall + caps_overall;
  float rtmc = tapCaps_r_overall + Mccaps_overall;
  float ftmc = tapCaps_f_overall + Mccaps_overall;
  rct.outCap_n_r[net_id] = rtc;
  rct.outCap_n_f[net_id] = ftc;
  rct.outCap_s_r[net_id] = (rtmc - rtc)/3;
  rct.outCap_s_f[net_id] = (ftmc - ftc)/3;
  */
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
  //__shared__ int currentOrder;
  //extern __shared__ int sprev[];
  //extern __shared__ float spres[];
  // Dynamic shared memory base pointer
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

    // TODO: num_nodes > 1024 (use for loop)
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
    int prev = rct.pid[n]; // not good?
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
        //__syncthreads();
      }
      // upstream
      if (tid == 0) {
        dnr[tid] = pres*lnr[tid];
        dnf[tid] = pres*lnf[tid];
        dsr[tid] = Mr*pres*lsr[tid];
        dsf[tid] = Mr*pres*lsf[tid];
      }
      //__syncthreads();
      for (int currentOrder = 1; currentOrder < num_nodes; currentOrder++) {
        __syncthreads();
        if (tid == currentOrder) {
          dnr[order] += dnr[prev] + pres*lnr[order];
          dnf[order] += dnf[prev] + pres*lnf[order];
          dsr[order] += dsr[prev] + Mr*pres*lsr[order];
          dsf[order] += dsf[prev] + Mr*pres*lsf[order];
        }
        //__syncthreads();
      }
      bnr[tid] *= dnr[tid];
      bnf[tid] *= dnf[tid];
      bsr[tid] *= dsr[tid];
      bsf[tid] *= dsf[tid];
      //__syncthreads();
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

    /*
    if (tid < num_nodes) {
      if (tid == 0) {
        dnr[tid] = pres*lnr[tid];
        dnf[tid] = pres*lnf[tid];
        dsr[tid] = Mr*pres*lsr[tid];
        dsf[tid] = Mr*pres*lsf[tid];
      }
      __syncthreads();
      for (int currentOrder = 1; currentOrder < num_nodes; currentOrder++) {
        __syncthreads();
        if (tid == currentOrder) {
          dnr[order] += dnr[prev] + pres*lnr[order];
          dnf[order] += dnf[prev] + pres*lnf[order];
          dsr[order] += dsr[prev] + Mr*pres*lsr[order];
          dsf[order] += dsf[prev] + Mr*pres*lsf[order];
        }
        __syncthreads();
      }
    }
    

    if (tid < num_nodes) {
      bnr[tid] *= dnr[tid];
      bnf[tid] *= dnf[tid];
      bsr[tid] *= dsr[tid];
      bsf[tid] *= dsf[tid];
      __syncthreads();
      for (int currentOrder = num_nodes-1; currentOrder > 0; currentOrder--) {
        __syncthreads();
        if (tid == currentOrder) { 
          bnr[prev] += bnr[order];
          bnf[prev] += bnf[order];
          bsr[prev] += bsr[order];
          bsf[prev] += bsf[order];
        }
        __syncthreads();
      }
    }

    if (tid < num_nodes) {
      for (int currentOrder = 1; currentOrder < num_nodes; currentOrder++) {
        __syncthreads();
        if (tid == currentOrder) { 
          bnr[order] += bnr[prev] + pres*bnr[order];
          bnf[order] += bnf[prev] + pres*bnf[order];
          bsr[order] += bsr[prev] + Mr*pres*bsr[order];
          bsf[order] += bsf[prev] + Mr*pres*bsf[order];
        }
        __syncthreads();
      }
    }

    if (tid < num_nodes) {
      float _dnr = dnr[tid];
      float _dnf = dnf[tid];
      float _dsr = dsr[tid];
      float _dsf = dsf[tid];
      snr[tid] = sqrtf(2*bnr[tid] - _dnr*_dnr);
      snf[tid] = sqrtf(2*bnf[tid] - _dnf*_dnf);
      ssr[tid] = sqrtf(2*bsr[tid] - _dsr*_dsr);
      ssf[tid] = sqrtf(2*bsf[tid] - _dsf*_dsf);
    }
    */
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
    //__syncthreads();

    /*
    if (blockDim.x >= 512) { 
      if (tid < 256) { 
        scaps[tid] += scaps[tid + 256]; 
        stcaps_r[tid] += stcaps_r[tid + 256];
        stcaps_f[tid] += stcaps_f[tid + 256];
      } __syncthreads(); 
    }
    if (blockDim.x >= 256) { 
      if (tid < 128) { 
        scaps[tid] += scaps[tid + 128];
        stcaps_r[tid] += stcaps_r[tid + 128];
        stcaps_f[tid] += stcaps_f[tid + 128];
      } 
      __syncthreads(); 
    }
    
    
    if (tid < 64) { 
      scaps[tid] += scaps[tid + 64]; 
      stcaps_r[tid] += stcaps_r[tid + 64];
      stcaps_f[tid] += stcaps_f[tid + 64];
    }
    
    
    if (tid < 32) { // warp
      scaps[tid] += scaps[tid + 32];
      scaps[tid] += scaps[tid + 16];
      scaps[tid] += scaps[tid + 8];
      scaps[tid] += scaps[tid + 4];
      scaps[tid] += scaps[tid + 2];
      scaps[tid] += scaps[tid + 1];
      stcaps_r[tid] += stcaps_r[tid + 32];
      stcaps_r[tid] += stcaps_r[tid + 16];
      stcaps_r[tid] += stcaps_r[tid + 8];
      stcaps_r[tid] += stcaps_r[tid + 4];
      stcaps_r[tid] += stcaps_r[tid + 2];
      stcaps_r[tid] += stcaps_r[tid + 1];
      stcaps_f[tid] += stcaps_f[tid + 32];
      stcaps_f[tid] += stcaps_f[tid + 16];
      stcaps_f[tid] += stcaps_f[tid + 8];
      stcaps_f[tid] += stcaps_f[tid + 4];
      stcaps_f[tid] += stcaps_f[tid + 2];
      stcaps_f[tid] += stcaps_f[tid + 1];
    }
    */

    /*
    if (tid == 0) {
      rct.outCap_n_r[net_id] = scaps[tid] + stcaps_r[tid];
      rct.outCap_n_f[net_id] = scaps[tid] + stcaps_f[tid];
      rct.outCap_s_r[net_id] = (Mc*scaps[tid] - scaps[tid])/3;
      rct.outCap_s_f[net_id] = (Mc*scaps[tid] - scaps[tid])/3;
    }
    */

  }

}

__global__ void compute_rc_timing2(RctCUDA rct, float Mc, float Mr) {
  unsigned int net_id = blockIdx.x * blockDim.x + threadIdx.x;
  unsigned int type_idx = threadIdx.y;
  if(net_id >= rct.num_nets) return;
  int nodes_st = rct.nodes_st[net_id]; 
  int nodes_en = rct.nodes_st[net_id + 1];
  int taps_st = rct.taps_st[net_id];
  int taps_en = rct.taps_st[net_id + 1];

  // TODO:
  /*
  const float4 cap_multiplier = make_float4(1.0f, 1.0f, Mc, Mc);

  // Reinterpret pointers (or change struct definition)
  float4* load_f4 = (float4*)rct.load;
  float4* beta_f4 = (float4*)rct.beta;
  // Assume outCap should also be float4 based on commented-out line
  float4* outCap_f4 = (float4*)rct.outCap;

  float4 net_outCap = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
  */


  // type id
  // 0,1 = nominal (f, r)
  // 2,3 = sense (f, r)
  
  float mc = 1.0f;
  float mr = 1.0f;
  if (type_idx > 1) {
    mc = Mc;
    mr = Mr;
  }

  // init
  //rct.outCap[4*net_id + type_idx] = 0.0f;
  float outCap = 0.0f;
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
  // don't need the cap look from port
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
    if (type_idx > 1) {
      float bn = rct.slew[4*tap + type_idx%2];
      float dn = rct.delay[4*tap + type_idx%2];
      rct.tapDelay[4*i + type_idx] = (rct.tapDelay[4*i + type_idx] - rct.delay[4*tap + type_idx%2])/3;
      rct.tapSlew[4*i + type_idx] = (rct.tapSlew[4*i + type_idx] - sqrtf(2*bn - dn*dn))/3;
    }
  }

  // Outcap
  rct.outCap[4*net_id + type_idx] = outCap;
  if (type_idx > 1) {
    rct.outCap[4*net_id + type_idx] = (outCap - rct.outCap[4*net_id + type_idx%2])/3;
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
  //int num_blocks = (rct_cuda.num_nets + num_threads - 1) / num_threads;
  //compute_rc_timing<<<num_blocks, num_threads>>>(rct_cuda, Mc, Mr);
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