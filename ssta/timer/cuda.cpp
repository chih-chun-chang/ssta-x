#include <ssta/timer/timer.hpp>

RctCUDA Timer::_copy_rct2gpu() {
  int num_nodes = _rct.nodes_st.back();
  int num_nets = _rct.names.size();

  RctCUDA rct_cuda;
  rct_cuda.num_nets = num_nets;
  rct_cuda.num_nodes = num_nodes;
  rct_cuda.num_edges = _rct.edges_s.size();

  // input
  allocateCopyCUDA(rct_cuda.nodes_st,  _rct.nodes_st.data(), _rct.nodes_st.size());
  allocateCopyCUDA(rct_cuda.edges_st,  _rct.edges_st.data(), _rct.edges_st.size());
  allocateCopyCUDA(rct_cuda.edges_s,   _rct.edges_s.data(),  _rct.edges_s.size());           
  allocateCopyCUDA(rct_cuda.edges_t,   _rct.edges_t.data(),  _rct.edges_t.size());
  allocateCopyCUDA(rct_cuda.ress,      _rct.ress.data(),     _rct.ress.size());
  allocateCopyCUDA(rct_cuda.caps,      _rct.caps.data(),     _rct.caps.size());
  allocateCopyCUDA(rct_cuda.ports,     _rct.ports.data(),    _rct.ports.size());
  allocateCopyCUDA(rct_cuda.taps_st,   _rct.taps_st.data(),  _rct.taps_st.size());
  allocateCopyCUDA(rct_cuda.taps,      _rct.taps.data(),     _rct.taps.size());
  allocateCopyCUDA(rct_cuda.tapCaps,   _rct.tapCaps.data(),  _rct.tapCaps.size());
  // bfs
  allocateCUDA(rct_cuda.distances, num_nodes,  int); 
  allocateCUDA(rct_cuda.counts,    num_nodes,  int); 
  allocateCUDA(rct_cuda.orders,    num_nodes,  int); 
  allocateCUDA(rct_cuda.outputs,   num_nodes,  int); 
  allocateCUDA(rct_cuda.pid,       num_nodes,  int);
  allocateCUDA(rct_cuda.pres,      num_nodes,  float); 
  // timing
  allocateCUDA(rct_cuda.load,  4*num_nodes, float);
  allocateCUDA(rct_cuda.delay, 4*num_nodes, float);
  allocateCUDA(rct_cuda.beta,  4*num_nodes, float);
  allocateCUDA(rct_cuda.slew,  4*num_nodes, float); 
  allocateCUDA(rct_cuda.tapDelay, 4*_rct.taps.size(), float);
  allocateCUDA(rct_cuda.tapSlew,  4*_rct.taps.size(), float);
  allocateCUDA(rct_cuda.outCap,   4*num_nets, float);
  return rct_cuda;
}

void Timer::_copy_rct2cpu(RctCUDA& rct_cuda) {
  int num_nodes = _rct.nodes_st.back();
  int num_nets = _rct.names.size();
  _deRct.outCap.resize(4*_rct.names.size());
  memcpyD2HCUDA(_deRct.outCap.data(), rct_cuda.outCap, 4*_rct.names.size());
  _deRct.tapDelay.resize(4*_rct.taps.size());
  memcpyD2HCUDA(_deRct.tapDelay.data(), rct_cuda.tapDelay, 4*_rct.taps.size());
  _deRct.tapSlew.resize(4*_rct.taps.size());
  memcpyD2HCUDA(_deRct.tapSlew.data(), rct_cuda.tapSlew, 4*_rct.taps.size());

}

TimingCUDA Timer::_copy_timing2gpu() {

  TimingCUDA timing_cuda;
  // cell
  allocateCopyCUDA(timing_cuda.cell_type,       _t.cell_type.data(),       _t.cell_type.size());
  allocateCopyCUDA(timing_cuda.cell_hasclk,    _t.cell_hasclk.data(),    _t.cell_hasclk.size());
  allocateCopyCUDA(timing_cuda.cell_level_ptr, _t.cell_level_ptr.data(), _t.cell_level_ptr.size());
  allocateCopyCUDA(timing_cuda.cell_blevel_ptr, _t.cell_blevel_ptr.data(), _t.cell_blevel_ptr.size());
  allocateCopyCUDA(timing_cuda.map_to_forward,  _t.map_to_forward.data(),  _t.map_to_forward.size());
  // tcell data
  allocateCopyCUDA(timing_cuda.tcell_timing_sense,          _t.tcell_timing_sense.data(),          _t.tcell_timing_sense.size());
  allocateCopyCUDA(timing_cuda.tcell_timing_data_slew,      _t.tcell_timing_data_slew.data(),      _t.tcell_timing_data_slew.size());
  allocateCopyCUDA(timing_cuda.tcell_timing_data_delay,     _t.tcell_timing_data_delay.data(),     _t.tcell_timing_data_delay.size());
  allocateCopyCUDA(timing_cuda.tcell_constraint_setup_data, _t.tcell_constraint_setup_data.data(), _t.tcell_constraint_setup_data.size());
  allocateCopyCUDA(timing_cuda.tcell_constraint_hold_type,  _t.tcell_constraint_hold_type.data(),  _t.tcell_constraint_hold_type.size());
  allocateCopyCUDA(timing_cuda.tcell_constraint_hold_data,  _t.tcell_constraint_hold_data.data(),  _t.tcell_constraint_hold_data.size());
  allocateCopyCUDA(timing_cuda.tcell_timing_ptr,            _t.tcell_timing_ptr.data(),            _t.tcell_timing_ptr.size());
  allocateCopyCUDA(timing_cuda.tcell_constraint_ptr,        _t.tcell_constraint_ptr.data(),         _t.tcell_constraint_ptr.size());
  // cell data
  allocateCopyCUDA(timing_cuda.cell_arch_sense, _t.cell_arch_sense.data(), _t.cell_arch_sense.size());
  allocateCopyCUDA(timing_cuda.cell_arch_data_slew, _t.cell_arch_data_slew.data(), _t.cell_arch_data_slew.size());
  allocateCopyCUDA(timing_cuda.cell_arch_data_delay, _t.cell_arch_data_delay.data(), _t.cell_arch_data_delay.size());
  allocateCopyCUDA(timing_cuda.cell_arch_ptr, _t.cell_arch_ptr.data(), _t.cell_arch_ptr.size());
  // node
  allocateCopyCUDA(timing_cuda.ipin,                  _t.ipin.data(),                   _t.ipin.size());
  allocateCopyCUDA(timing_cuda.ipin_ptr,              _t.ipin_ptr.data(),               _t.ipin_ptr.size());
  allocateCopyCUDA(timing_cuda.ipin_timing_data_slew, _t.ipin_timing_data_slew.data(),  _t.ipin_timing_data_slew.size());
  allocateCopyCUDA(timing_cuda.ipin_timing_data_at,   _t.ipin_timing_data_at.data(),    _t.ipin_timing_data_at.size());
  allocateCopyCUDA(timing_cuda.ipin_timing_data_rat,  _t.ipin_timing_data_rat.data(),    _t.ipin_timing_data_rat.size());
  allocateCopyCUDA(timing_cuda.wp,              _t.wp.data(),              _t.wp.size());
  allocateCopyCUDA(timing_cuda.nipin,           _t.nipin.data(),           _t.nipin.size());
  allocateCopyCUDA(timing_cuda.nipin_ptr,       _t.nipin_ptr.data(),       _t.nipin_ptr.size());
  allocateCopyCUDA(timing_cuda.bnipin_ptr,      _t.bnipin_ptr.data(),      _t.bnipin_ptr.size());
  allocateCopyCUDA(timing_cuda.bnipin_ptr_each_gate, _t.bnipin_ptr_each_gate.data(), _t.bnipin_ptr_each_gate.size());
  allocateCopyCUDA(timing_cuda.opin,                  _t.opin.data(),                   _t.opin.size());
  allocateCopyCUDA(timing_cuda.opin_ptr,              _t.opin_ptr.data(),               _t.opin_ptr.size());
  allocateCUDA(timing_cuda.opin_timing_data_slew,     _t.opin_timing_data_slew.size(), float);
  allocateCUDA(timing_cuda.opin_timing_data_at,       _t.opin_timing_data_at.size(),   float);
  allocateCUDA(timing_cuda.opin_timing_data_rat,      _t.opin_timing_data_rat.size(),  float);
  allocateCUDA(timing_cuda.ipint_timing_data_slew, _t.ipint_timing_data_slew.size(), float);
  allocateCUDA(timing_cuda.ipint_timing_data_at,   _t.ipint_timing_data_at.size(), float);
  allocateCUDA(timing_cuda.ipint_timing_data_rat,   _t.ipint_timing_data_rat.size(), float);
  allocateCopyCUDA(timing_cuda.ipint_ptr,              _t.ipint_ptr.data(),               _t.ipint_ptr.size());
  allocateCopyCUDA(timing_cuda.opin_index,            _t.opin_index.data(),              _t.opin_index.size());
  return timing_cuda;
}
  
// free
void Timer::_close_cuda(RctCUDA& rct_cuda, TimingCUDA& timing_cuda) {
  // rct
  destroyCUDA(rct_cuda.nodes_st);
  destroyCUDA(rct_cuda.edges_st);
  destroyCUDA(rct_cuda.edges_s);           
  destroyCUDA(rct_cuda.edges_t);
  destroyCUDA(rct_cuda.ress);
  destroyCUDA(rct_cuda.caps);
  destroyCUDA(rct_cuda.ports);
  destroyCUDA(rct_cuda.taps_st);
  destroyCUDA(rct_cuda.taps);
  destroyCUDA(rct_cuda.tapCaps);
  
  destroyCUDA(rct_cuda.distances); 
  destroyCUDA(rct_cuda.counts); 
  destroyCUDA(rct_cuda.orders); 
  destroyCUDA(rct_cuda.outputs); 
  destroyCUDA(rct_cuda.pid);
  destroyCUDA(rct_cuda.pres);

  destroyCUDA(rct_cuda.load);
  destroyCUDA(rct_cuda.delay);
  destroyCUDA(rct_cuda.beta);
  destroyCUDA(rct_cuda.slew); 
  destroyCUDA(rct_cuda.tapDelay);
  destroyCUDA(rct_cuda.tapSlew);
  destroyCUDA(rct_cuda.outCap);

  // timing
  destroyCUDA(timing_cuda.cell_hasclk);
  destroyCUDA(timing_cuda.cell_level_ptr);
  destroyCUDA(timing_cuda.cell_blevel_ptr);
  destroyCUDA(timing_cuda.map_to_forward);
  destroyCUDA(timing_cuda.tcell_timing_sense);
  destroyCUDA(timing_cuda.tcell_timing_data_slew);
  destroyCUDA(timing_cuda.tcell_timing_data_delay);
  destroyCUDA(timing_cuda.tcell_constraint_setup_data);
  destroyCUDA(timing_cuda.tcell_constraint_hold_type);
  destroyCUDA(timing_cuda.tcell_constraint_hold_data);
  destroyCUDA(timing_cuda.tcell_timing_ptr);
  destroyCUDA(timing_cuda.tcell_constraint_ptr);

  destroyCUDA(timing_cuda.cell_arch_sense);
  destroyCUDA(timing_cuda.cell_arch_data_slew);
  destroyCUDA(timing_cuda.cell_arch_data_delay);
  destroyCUDA(timing_cuda.cell_arch_ptr);

  destroyCUDA(timing_cuda.ipin);
  destroyCUDA(timing_cuda.ipin_ptr);
  destroyCUDA(timing_cuda.ipin_timing_data_slew);
  destroyCUDA(timing_cuda.ipin_timing_data_at);
  destroyCUDA(timing_cuda.ipin_timing_data_rat);
  destroyCUDA(timing_cuda.opin_timing_data_slew);
  destroyCUDA(timing_cuda.opin_timing_data_at);
  destroyCUDA(timing_cuda.opin_timing_data_rat);
  destroyCUDA(timing_cuda.ipint_timing_data_slew);
  destroyCUDA(timing_cuda.ipint_timing_data_at);
  destroyCUDA(timing_cuda.ipint_timing_data_rat);

  destroyCUDA(timing_cuda.wp);
  destroyCUDA(timing_cuda.nipin);
  destroyCUDA(timing_cuda.nipin_ptr);
  destroyCUDA(timing_cuda.bnipin_ptr);
  destroyCUDA(timing_cuda.bnipin_ptr_each_gate);
  destroyCUDA(timing_cuda.opin);
  destroyCUDA(timing_cuda.opin_ptr);
  destroyCUDA(timing_cuda.ipint_ptr);
  destroyCUDA(timing_cuda.opin_index);
}

void Timer::_run_cuda() {

  float Mr = (*_celllib)->metal3ResScaleFactor;
  float Mc = (*_celllib)->metal3CapScaleFactor;

  // timing
  cudaEvent_t start, stop;
  float transfer = 0;
  float rc = 0;
  float prop = 0;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  cudaEventRecord(start);
  RctCUDA rct_cuda = _copy_rct2gpu();
  TimingCUDA timing_cuda = _copy_timing2gpu();
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&transfer, start, stop);
  std::cout << "Data transfer time: " << transfer << " ms" << std::endl;


  cudaEventRecord(start);
  _bfs(rct_cuda);
  _compute_rc_timing2(rct_cuda, Mc, Mr);
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&rc, start, stop);
  std::cout << "RC execution time: " << rc << " ms" << std::endl;
  

  // PI
  float fprop_ms = 0;
  cudaEventRecord(start);

  _addWireParamsForward(timing_cuda, rct_cuda, _inputs.size(), 0, _inputs.size(), -1);

  int start_idx = _t.ipin_level_ptr[0];
  int num_pins = _t.ipin_level_ptr[1] - start_idx;

  int nipin_start = 0;
  nipin_start=_inputs.size();
  // prop
  for (const auto& [level, insts] : _forward) {
    _getTDataSense(timing_cuda, _inputs.size(), insts.size(), level, 0);
    _addDelayParam(timing_cuda, rct_cuda, _inputs.size(), insts.size(), level, 0, 0);
    _max(timing_cuda, _inputs.size(), insts.size(), level);
    _addWireParamsForward(timing_cuda, rct_cuda, _inputs.size(), nipin_start, _t.opin_level_ptr[level+1]-_t.opin_level_ptr[level], level);
    nipin_start+=insts.size();
  }
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&fprop_ms, start, stop);
  std::cout << "fprop time: " << fprop_ms << " ms" << std::endl;

  cudaDeviceSynchronize();


  float bprop_ms = 0;
  cudaEventRecord(start);
  nipin_start = 0;
  for (const auto& [level, insts] : _backward) {    
    _addWireParamsBackward(timing_cuda, rct_cuda, _inputs.size(), nipin_start, insts.size(), level);
    _getTDataSense(timing_cuda, _inputs.size(), insts.size(), level, 2); // dir == 2 to skip clk instance
    _addDelayParam(timing_cuda, rct_cuda, _inputs.size(), insts.size(), level, 1, _clockTP);
    _getTDataSense(timing_cuda, _inputs.size(), insts.size(), level, 1);
    _maxR(timing_cuda, _inputs.size(), insts.size(), level);
    nipin_start+=insts.size();
  }
  _addWireParamsBackward(timing_cuda, rct_cuda, _inputs.size(), 0, _inputs.size(), -1);
  
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&bprop_ms, start, stop);
  std::cout << "bprop time: " << bprop_ms << " ms" << std::endl;
  
  cudaError_t err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    printf("CUDA Kernel Error: %s\n", cudaGetErrorString(err));
  }

  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&prop, start, stop);
  _copy_rct2cpu(rct_cuda);
  std::cout << "Total time: " << transfer + rc + fprop_ms + bprop_ms << " ms" << std::endl;

  _close_cuda(rct_cuda, timing_cuda);
  
  cudaEventDestroy(start);
  cudaEventDestroy(stop);
}

RctCUDA Timer::computeWireTiming() {

  float Mr = (*_celllib)->metal3ResScaleFactor;
  float Mc = (*_celllib)->metal3CapScaleFactor;

  RctCUDA rct_cuda = _copy_rct2gpu();

  _bfs(rct_cuda);
  _compute_rc_timing2(rct_cuda, Mc, Mr);
  
  return rct_cuda;
}

TimingResult Timer::run_timed() {
  _build_connections(20); // forward and backward levels  
  _get_arch_timing();
  float Mr = (*_celllib)->metal3ResScaleFactor;
  float Mc = (*_celllib)->metal3CapScaleFactor;

  // timing
  cudaEvent_t start, stop;

  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  RctCUDA rct_cuda = _copy_rct2gpu();
  TimingCUDA timing_cuda = _copy_timing2gpu();
  _bfs(rct_cuda);
  _compute_rc_timing2(rct_cuda, Mc, Mr);
 
  float fprop_ms = 0;
  cudaEventRecord(start);
  _addWireParamsForward(timing_cuda, rct_cuda, _inputs.size(), 0, _inputs.size(), -1);
  int start_idx = _t.ipin_level_ptr[0];
  int num_pins = _t.ipin_level_ptr[1] - start_idx;
  int nipin_start = 0;
  nipin_start=_inputs.size();
  for (const auto& [level, insts] : _forward) {
    _getTDataSense(timing_cuda, _inputs.size(), insts.size(), level, 0);
    _addDelayParam(timing_cuda, rct_cuda, _inputs.size(), insts.size(), level, 0, 0);
    _max(timing_cuda, _inputs.size(), insts.size(), level);
    _addWireParamsForward(timing_cuda, rct_cuda, _inputs.size(), nipin_start, _t.opin_level_ptr[level+1]-_t.opin_level_ptr[level], level);
    nipin_start+=insts.size();
  }
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&fprop_ms, start, stop);

  float bprop_ms = 0;
  cudaEventRecord(start);
  nipin_start = 0;
  for (const auto& [level, insts] : _backward) {
    _addWireParamsBackward(timing_cuda, rct_cuda, _inputs.size(), nipin_start, insts.size(), level);
    _getTDataSense(timing_cuda, _inputs.size(), insts.size(), level, 2); // dir == 2 to skip clk instance
    _addDelayParam(timing_cuda, rct_cuda, _inputs.size(), insts.size(), level, 1, _clockTP);
    _getTDataSense(timing_cuda, _inputs.size(), insts.size(), level, 1);
    _maxR(timing_cuda, _inputs.size(), insts.size(), level);
    nipin_start+=insts.size();
  }
  _addWireParamsBackward(timing_cuda, rct_cuda, _inputs.size(), 0, _inputs.size(), -1);
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&bprop_ms, start, stop);

  cudaError_t err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    printf("CUDA Kernel Error: %s\n", cudaGetErrorString(err));
  }

  _copy_rct2cpu(rct_cuda);
  cudaEventDestroy(start);
  cudaEventDestroy(stop);

  _close_cuda(rct_cuda, timing_cuda);

  return {(double)fprop_ms, (double)bprop_ms};
}
