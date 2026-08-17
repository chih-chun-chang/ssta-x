#include <ssta/timer/timer.hpp>

RctCUDA Timer::_copy_rct2gpu() {
  int num_nodes = _rct.nodes_st.back();
  int num_nets = _rct.names.size();

  RctCUDA rct_cuda;
  rct_cuda.num_nets = num_nets;
  rct_cuda.num_nodes = num_nodes;
  rct_cuda.num_edges = _rct.edges_s.size();

  //std::cout << "num_nodes:" << num_nodes << std::endl;
  //std::cout << "num_edges:" << rct_cuda.num_edges << std::endl;

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
  // debug
  int num_nodes = _rct.nodes_st.back();
  int num_nets = _rct.names.size();
  
  /*
  _deRct.distances.resize(num_nodes);
  memcpyD2HCUDA(_deRct.distances.data(), rct_cuda.distances, num_nodes);
  _deRct.counts.resize(num_nodes);
  memcpyD2HCUDA(_deRct.counts.data(), rct_cuda.counts, num_nodes);
  
  _deRct.orders.resize(num_nodes);
  memcpyD2HCUDA(_deRct.orders.data(), rct_cuda.orders, num_nodes);

  _deRct.outputs.resize(num_nodes);
  memcpyD2HCUDA(_deRct.outputs.data(), rct_cuda.outputs, num_nodes);

  _deRct.pid.resize(num_nodes);
  memcpyD2HCUDA(_deRct.pid.data(), rct_cuda.pid, num_nodes)
  _deRct.pres.resize(num_nodes);
  memcpyD2HCUDA(_deRct.pres.data(), rct_cuda.pres, num_nodes)
  
  
  //_deRct.load.resize(4*num_nodes);
  //memcpyD2HCUDA(_deRct.load.data(), rct_cuda.load, 4*num_nodes);
  _deRct.delay.resize(4*num_nodes);
  memcpyD2HCUDA(_deRct.delay.data(), rct_cuda.delay, 4*num_nodes);
  _deRct.slew.resize(4*num_nodes);
  memcpyD2HCUDA(_deRct.slew.data(), rct_cuda.slew, 4*num_nodes);
  */
  //_deRct.tapCaps.resize(2*_rct.taps.size());
  //memcpyD2HCUDA(_deRct.tapCaps.data(), rct_cuda.tapCaps, 2*_rct.taps.size());
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
  //allocateCopyCUDA(timing_cuda.cell_ptr,       _t.cell_ptr.data(),       _t.cell_ptr.size());
  allocateCopyCUDA(timing_cuda.cell_type,       _t.cell_type.data(),       _t.cell_type.size());
  allocateCopyCUDA(timing_cuda.cell_hasclk,    _t.cell_hasclk.data(),    _t.cell_hasclk.size());
  allocateCopyCUDA(timing_cuda.cell_level_ptr, _t.cell_level_ptr.data(), _t.cell_level_ptr.size());
  allocateCopyCUDA(timing_cuda.cell_blevel_ptr, _t.cell_blevel_ptr.data(), _t.cell_blevel_ptr.size());
  allocateCopyCUDA(timing_cuda.map_to_forward,  _t.map_to_forward.data(),  _t.map_to_forward.size());
  // tcell data
  allocateCopyCUDA(timing_cuda.tcell_timing_sense,          _t.tcell_timing_sense.data(),          _t.tcell_timing_sense.size());
  allocateCopyCUDA(timing_cuda.tcell_timing_data_slew,      _t.tcell_timing_data_slew.data(),      _t.tcell_timing_data_slew.size());
  allocateCopyCUDA(timing_cuda.tcell_timing_data_delay,     _t.tcell_timing_data_delay.data(),     _t.tcell_timing_data_delay.size());
  //allocateCopyCUDA(timing_cuda.tcell_constraint_setup_type, _t.tcell_constraint_setup_type.data(), _t.tcell_constraint_setup_type.size());
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
  //allocateCopyCUDA(timing_cuda.ipin_level_ptr,        _t.ipin_level_ptr.data(),         _t.ipin_level_ptr.size());
  allocateCopyCUDA(timing_cuda.ipin_timing_data_slew, _t.ipin_timing_data_slew.data(),  _t.ipin_timing_data_slew.size());
  allocateCopyCUDA(timing_cuda.ipin_timing_data_at,   _t.ipin_timing_data_at.data(),    _t.ipin_timing_data_at.size());
  allocateCopyCUDA(timing_cuda.ipin_timing_data_rat,  _t.ipin_timing_data_rat.data(),    _t.ipin_timing_data_rat.size());
  allocateCopyCUDA(timing_cuda.wp,              _t.wp.data(),              _t.wp.size());
  allocateCopyCUDA(timing_cuda.nipin,           _t.nipin.data(),           _t.nipin.size());
  allocateCopyCUDA(timing_cuda.nipin_ptr,       _t.nipin_ptr.data(),       _t.nipin_ptr.size());
  allocateCopyCUDA(timing_cuda.bnipin_ptr,      _t.bnipin_ptr.data(),      _t.bnipin_ptr.size());
  allocateCopyCUDA(timing_cuda.bnipin_ptr_each_gate, _t.bnipin_ptr_each_gate.data(), _t.bnipin_ptr_each_gate.size());
  //allocateCopyCUDA(timing_cuda.nipin_level_ptr, _t.nipin_level_ptr.data(), _t.nipin_level_ptr.size());
  allocateCopyCUDA(timing_cuda.opin,                  _t.opin.data(),                   _t.opin.size());
  allocateCopyCUDA(timing_cuda.opin_ptr,              _t.opin_ptr.data(),               _t.opin_ptr.size());
  allocateCUDA(timing_cuda.opin_timing_data_slew,     _t.opin_timing_data_slew.size(), float);
  allocateCUDA(timing_cuda.opin_timing_data_at,       _t.opin_timing_data_at.size(),   float);
  allocateCUDA(timing_cuda.opin_timing_data_rat,      _t.opin_timing_data_rat.size(),  float);
  //allocateCUDA(timing_cuda.output_timing_data_slew,   _t.output_timing_data_slew.size(), float);
  //allocateCUDA(timing_cuda.output_timing_data_at,     _t.output_timing_data_at.size(),   float);
  //allocateCopyCUDA(timing_cuda.output_timing_data_rat, _t.output_timing_data_rat.data(), _t.output_timing_data_rat.size());
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
  //destroyCUDA(timing_cuda.cell_ptr);
  destroyCUDA(timing_cuda.cell_hasclk);
  destroyCUDA(timing_cuda.cell_level_ptr);
  destroyCUDA(timing_cuda.cell_blevel_ptr);
  destroyCUDA(timing_cuda.map_to_forward);

  destroyCUDA(timing_cuda.tcell_timing_sense);
  destroyCUDA(timing_cuda.tcell_timing_data_slew);
  destroyCUDA(timing_cuda.tcell_timing_data_delay);
  //destroyCUDA(timing_cuda.tcell_constraint_setup_type);
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
  //destroyCUDA(timing_cuda.ipin_level_ptr);
  destroyCUDA(timing_cuda.ipin_timing_data_slew);
  destroyCUDA(timing_cuda.ipin_timing_data_at);
  destroyCUDA(timing_cuda.ipin_timing_data_rat);
  destroyCUDA(timing_cuda.opin_timing_data_slew);
  destroyCUDA(timing_cuda.opin_timing_data_at);
  destroyCUDA(timing_cuda.opin_timing_data_rat);
  //destroyCUDA(timing_cuda.output_timing_data_slew);
  //destroyCUDA(timing_cuda.output_timing_data_at);
  //destroyCUDA(timing_cuda.output_timing_data_rat);
  destroyCUDA(timing_cuda.ipint_timing_data_slew);
  destroyCUDA(timing_cuda.ipint_timing_data_at);
  destroyCUDA(timing_cuda.ipint_timing_data_rat);

  destroyCUDA(timing_cuda.wp);
  destroyCUDA(timing_cuda.nipin);
  destroyCUDA(timing_cuda.nipin_ptr);
  destroyCUDA(timing_cuda.bnipin_ptr);
  destroyCUDA(timing_cuda.bnipin_ptr_each_gate);
  //destroyCUDA(timing_cuda.nipin_level_ptr);
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
  
  /*
  // TODO: check RC timing results
  //_deRct.tapCaps.resize(2*_rct.taps.size());
  //memcpyD2HCUDA(_deRct.tapCaps.data(), rct_cuda.tapCaps, 2*_rct.taps.size());
  _deRct.outCap.resize(4*_rct.names.size());
  memcpyD2HCUDA(_deRct.outCap.data(), rct_cuda.outCap, 4*_rct.names.size());
  _deRct.tapDelay.resize(4*_rct.taps.size());
  memcpyD2HCUDA(_deRct.tapDelay.data(), rct_cuda.tapDelay, 4*_rct.taps.size());
  _deRct.tapSlew.resize(4*_rct.taps.size());
  memcpyD2HCUDA(_deRct.tapSlew.data(), rct_cuda.tapSlew, 4*_rct.taps.size());
  for (int i = 0; i < _rct.ports_name.size(); ++i) {
    std::cout << "\nPort: " << _rct.ports_name[i] << "\n";
    std::cout << "******* out cap *******\n";
    for (int j = 0; j < _rct.taps_st[i+1] - _rct.taps_st[i]; ++j) {
      int idx = _rct.taps_st[i] + j;
      std::cout << "tap[" << j << "] ";
      std::cout << _rct.taps_name[idx] << " ";
      std::cout << _deRct.outCap[4*idx] << " ";
      std::cout << _deRct.outCap[4*idx + 1] << " ";
      std::cout << _deRct.outCap[4*idx + 2] << " ";
      std::cout << _deRct.outCap[4*idx + 3] << "\n";
    }
    std::cout << "******* tap delay *******\n";
    for (int j = 0; j < _rct.taps_st[i+1] - _rct.taps_st[i]; ++j) {
      int idx = _rct.taps_st[i] + j;
      std::cout << "tap[" << j << "] ";
      std::cout << _rct.taps_name[idx] << " ";
      std::cout << _deRct.tapDelay[4*idx] << " ";
      std::cout << _deRct.tapDelay[4*idx+1] << " ";
      std::cout << _deRct.tapDelay[4*idx+2] << " ";
      std::cout << _deRct.tapDelay[4*idx+3] << "\n";
      //
    }
    std::cout << "******* tap slew *******\n";
    for (int j = 0; j < _rct.taps_st[i+1] - _rct.taps_st[i]; ++j) {
      int idx = _rct.taps_st[i] + j;
      std::cout << "tap[" << j << "] ";
      std::cout << _rct.taps_name[idx] << " ";
      std::cout << _deRct.tapSlew[4*idx] << " ";
      std::cout << _deRct.tapSlew[4*idx+1] << " ";
      std::cout << _deRct.tapSlew[4*idx+2] << " ";
      std::cout << _deRct.tapSlew[4*idx+3] << "\n";
      //
    }
    std::cout << std::endl;
  }
  */

  
  /*
  std::cout << "\n\n\n";
  int nipin_start = 0;
  std::cout << "Inputs: \n";
  for (int i = 0; i < _inputs.size(); ++i) {
    std::cout << _inputs[i];
    std::cout << "\n port->taps: \n    ";
    std::cout << _rct.ports_name[_t.wp[i]] << " -> ";
    std::cout << " port id: " << _t.wp[i] << " \n";
    for (int j = _rct.taps_st[_t.wp[i]]; j < _rct.taps_st[_t.wp[i]+1]; ++j) {
      std::cout << _rct.taps_name[j] << " ";
    }
    std::cout << "\n next ipins: \n    ";
    int nipin_beg = _t.nipin_ptr[i];
    int nipin_end = _t.nipin_ptr[i + 1];
    std::cout << "nipin_beg: " << nipin_beg << "\n";
    for (int j = nipin_beg; j < nipin_end; ++j) {
      std::cout << _t.ipin_string[_t.nipin[j]] << " ";
    }
    std::cout << "\n\n";
    nipin_start++;
  }
  // prop
  std::cout << "\n\n\n";
  //int nipin_start = 5; // number of input pins
  int out_index = 0;
  for (const auto& [level, insts] : _forward) {
    std::cout << "Level " << level << "'s instance: \n";
    int ipin_start = _t.cell_level_ptr[level];
    int ipin_end = _t.cell_level_ptr[level + 1];
    for (int i = 0; i < insts.size(); ++i) {
      std::cout << insts[i] << "\n";
      int cell_type = _t.cell_type[ipin_start + i];
      int hasclk = _t.cell_hasclk[ipin_start + i];
      std::cout << "  cell_type: " << cell_type << " hasclk: " << hasclk << "\n";
      if (hasclk) {
        //cell_timing_ptr += cl->lib[cell]->numoPins;
        //cell_constraint_ptr += cl->lib[cell]->numoPins;
      }
      else {
        int cell_arch_ptr_beg = _t.cell_arch_ptr[cell_type];
        int cell_arch_ptr_end = _t.cell_arch_ptr[cell_type + 1];
        std::cout << "  cell_arch_ptr: " << cell_arch_ptr_beg << "  " << cell_arch_ptr_end << "\n";
        for (int j = cell_arch_ptr_beg; j < cell_arch_ptr_end; ++j) {
          std::cout << cell::to_string(_t.cell_arch_sense[j]) << " ";
        }
        std::cout << std::endl;
      }
      std::cout << " ipins: \n    ";
      int ibeg = _t.ipin_ptr[ipin_start + i + 5]; // TODO: num inputs
      int iend = _t.ipin_ptr[ipin_start + i + 1 + 5];
      for (int j = ibeg; j < iend; ++j) {
        std::cout << _t.ipin_string[_t.ipin[j]] << " ";
      }
      std::cout << "\n opins: \n    ";
      int obeg = _t.opin_ptr[ipin_start + i];
      int oend = _t.opin_ptr[ipin_start + i + 1];
      for (int j = obeg; j < oend; ++j) {
        std::cout << _t.opin_string[j] << " ";
      }
      std::cout << "\n port->taps: \n    ";
      for (int j = obeg; j < oend; ++j) {
        std::cout << _rct.ports_name[_t.wp[5+j]] << " -> ";
        for (int k = _rct.taps_st[_t.wp[5+j]]; k < _rct.taps_st[_t.wp[5+j]+1]; ++k) {
          std::cout << _rct.taps_name[k] << " ";
        }
      }
      std::cout << "\n next ipins: \n    ";
      int nipin_beg = _t.nipin_ptr[nipin_start + i];
      int nipin_end = _t.nipin_ptr[nipin_start + i + 1];
      for (int j = nipin_beg; j < nipin_end; ++j) {
        std::cout << _t.ipin_string[_t.nipin[j]] << " ";
      }
      if (nipin_beg == nipin_end) {
        for (int j = obeg; j < oend; ++j) {
          std::cout << _t.output_string[out_index] << " ";
          out_index++;
        }
      }
      std::cout << "\n\n\n";
    }
    nipin_start+=insts.size();
    std::cout << std::endl;
  }
  */

  //int nipin_start = 0;


  // PI
  float fprop_ms = 0;
  cudaEventRecord(start);

  _addWireParamsForward(timing_cuda, rct_cuda, _inputs.size(), 0, _inputs.size(), -1);


  int start_idx = _t.ipin_level_ptr[0];
  int num_pins = _t.ipin_level_ptr[1] - start_idx;

  int nipin_start = 0;
  nipin_start=_inputs.size();
  // prop
  //for (const auto& optr : _t.opin_level_ptr) std::cout << optr << " ";
  //std::cout << std::endl;
  //int max_key = _forward.rbegin()->first;
  //std::cout << "The maximum level is: " << max_key << std::endl;
  
  for (const auto& [level, insts] : _forward) {
    //std::cout << "level: " << level << std::endl;
    //for (const auto& ins : insts) {
    //  std::cout << ins << " ";
    //}
    //std::cout << "\n";
    
    //std::cout << "_getTDataSense:\n";
    _getTDataSense(timing_cuda, _inputs.size(), insts.size(), level, 0);
    //if (level == 9) break;

    //std::cout << "_addDelayParam:\n";
    _addDelayParam(timing_cuda, rct_cuda, _inputs.size(), insts.size(), level, 0, 0);
    //if (level == 2) break;

    //std::cout << "_max\n";
    _max(timing_cuda, _inputs.size(), insts.size(), level);

    //std::cout << "_addWireParamsForward:\n";
    _addWireParamsForward(timing_cuda, rct_cuda, _inputs.size(), nipin_start, _t.opin_level_ptr[level+1]-_t.opin_level_ptr[level], level);

    nipin_start+=insts.size();
    
    //if (level == 3) break;
  }
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&fprop_ms, start, stop);
  std::cout << "fprop time: " << fprop_ms << " ms" << std::endl;

  cudaDeviceSynchronize();
  //std::cout << "fprop done\n";

  /*
  std::vector<float> at(_t.ipin_timing_data_at.size());
  std::vector<float> at2(_t.ipint_timing_data_at.size());
  std::vector<float> slew(_t.ipin_timing_data_slew.size());
  std::vector<float> slew2(_t.ipint_timing_data_slew.size());
  memcpyD2HCUDA(at.data(), timing_cuda.ipin_timing_data_at, _t.ipin_timing_data_at.size());
  //memcpyD2HCUDA(at2.data(), timing_cuda.ipint_timing_data_at, _t.ipint_timing_data_at.size());
  memcpyD2HCUDA(slew.data(), timing_cuda.ipin_timing_data_slew, _t.ipin_timing_data_slew.size());
  //memcpyD2HCUDA(slew2.data(), timing_cuda.ipint_timing_data_slew, _t.ipint_timing_data_slew.size());
  
  std::cout << "ipin at:\n";
  for (int i = 0; i < at.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
    //if (_t.ipin_string[j] == "inst_15:CK") {
      if (ii == 0)
        std::cout << j << " " << _t.ipin_string[j] << "\n";
      if (i % 8 == 0) {
        std::cout << i << " " << at[i] << "\n";
      }
    //}
  }
  std::cout << "\nipin slew:\n";
  for (int i = 0; i < slew.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
   //if (_t.ipin_string[j] == "inst_98:A") {
      if (ii == 0)
        std::cout << j << " " << _t.ipin_string[j] << "\n";
      if (i % 8 == 0) {
        std::cout << i << " " << slew[i] << "\n";
      }
    //}
  }


  
  std::cout << "\nipint at:\n";
  for (const auto& [level, insts] : _forward) {
    for (int i = 0; i < insts.size(); ++i) {
      if (insts[i] == "inst_98") {
        int beg = _t.ipint_ptr[_t.cell_level_ptr[level] + i];
        int numop = _t.opin_ptr[_t.cell_level_ptr[level] + i + 1] - _t.opin_ptr[_t.cell_level_ptr[level] + i];
        int numip = _t.ipin_ptr[_t.cell_level_ptr[level] + i + 1 + _inputs.size()] - _t.ipin_ptr[_t.cell_level_ptr[level] + i + _inputs.size()];
        for (int j = 0; j < numop; ++j) {
          for (int k = 0; k < numip; ++k) {
            int idx = beg + j*numip + k;
            for (int l = 0; l < 32; ++l) {
              if (l % 8 == 0)
                std::cout << "ipint[" << idx*32+l << "] " << at2[idx*32 + l] << "\n";
            }
          }
        }
      }  
    }
  }

  std::cout << "\nipint slew:\n";
  for (const auto& [level, insts] : _forward) {
    for (int i = 0; i < insts.size(); ++i) {
      if (insts[i] == "inst_98") {
        int beg = _t.ipint_ptr[_t.cell_level_ptr[level] + i];
        int numop = _t.opin_ptr[_t.cell_level_ptr[level] + i + 1] - _t.opin_ptr[_t.cell_level_ptr[level] + i];
        int numip = _t.ipin_ptr[_t.cell_level_ptr[level] + i + 1 + _inputs.size()] - _t.ipin_ptr[_t.cell_level_ptr[level] + i + _inputs.size()];
        for (int j = 0; j < numop; ++j) {
          for (int k = 0; k < numip; ++k) {
            int idx = beg + j*numip + k;
            for (int l = 0; l < 32; ++l) {
              if (l % 8 == 0)
                std::cout << "ipint[" << idx*32+l << "] " << slew2[idx*32 + l] << "\n";
            }
          }
        }
      }  
    }
  }
  */
  
  /*
  std::cout << "\nopin at:\n";
  std::vector<float> oat(_t.opin_timing_data_at.size());
  std::vector<float> oslew(_t.opin_timing_data_slew.size());
  memcpyD2HCUDA(oat.data(), timing_cuda.opin_timing_data_at, _t.opin_timing_data_at.size());
  memcpyD2HCUDA(oslew.data(), timing_cuda.opin_timing_data_slew, _t.opin_timing_data_slew.size());
  for (int i = 0; i < oat.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
    //if (_t.opin_string[j] == "inst_20:Z") {
      if (ii == 0) std::cout << "\nat " << _t.opin_string[j] << "\n";
      if (i % 8 == 0) {
        std::cout << oat[i] << "\n";
      }
    //}
  }
  for (int i = 0; i < oat.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
    //if (_t.opin_string[j] == "inst_21:Z") {
      if (ii == 0)
        std::cout << j << " " << _t.opin_string[j] << "\n";
      if (i % 8 == 0) {
        std::cout << i << " " << oslew[i] << "\n";
      }
    //}
  }
  */

  
  /*
  std::vector<float> out(_t.output_timing_data_at.size());
  std::vector<float> os(_t.output_timing_data_slew.size());
  memcpyD2HCUDA(out.data(), timing_cuda.output_timing_data_at, _t.output_timing_data_at.size());
  memcpyD2HCUDA(os.data(), timing_cuda.output_timing_data_slew, _t.output_timing_data_slew.size());
  for (int i = 0; i < out.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
    if (i == 0)
      std::cout << "\nat " << _t.output_string[j] << " ";
    if (i % 32 == 0 && i != 0) {
      std::cout << "\nat " << _t.output_string[j] << " ";
    }
    if (ii % 8 == 0) {
      std::cout << out[i] << " ";
    }
  }
  std::cout << "\n";
  for (int i = 0; i < os.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
    if (i == 0)
      std::cout << "\nslew " << _t.output_string[j] << " ";
    if (i % 32 == 0 && i != 0) {
      std::cout << "\nslew " << _t.output_string[j] << " ";
    }
    if (ii % 8 == 0) {
      std::cout << os[i] << " ";
    }
  }
  */


  float bprop_ms = 0;
  
  //std::cout << "backward:\n";
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
  


  /*
  //TODO: check input rat
  std::vector<float> rat(_t.ipin_timing_data_rat.size());
  memcpyD2HCUDA(rat.data(), timing_cuda.ipin_timing_data_rat, _t.ipin_timing_data_rat.size());
  for (int i = 0; i < _inputs.size(); ++i) {
    std::cout << "rat ";
    std::cout << _t.ipin_string[i] << " ";
    std::cout << rat[i*32] << " ";
    std::cout << rat[i*32+8] << " ";
    std::cout << rat[i*32+16] << " ";
    std::cout << rat[i*32+24] << "\n";
  }
  

  //for (const auto& opin_string : _t.opin_string) {
  //  std::cout << opin_string << " ";
  //}
  //std::cout << "\n\n";

  
  std::vector<float> rat2(_t.opin_timing_data_rat.size());
  std::vector<float> irat(_t.ipin_timing_data_rat.size());
  std::vector<float> irat2(_t.ipint_timing_data_rat.size());
  memcpyD2HCUDA(rat2.data(), timing_cuda.opin_timing_data_rat, _t.opin_timing_data_rat.size());
  memcpyD2HCUDA(irat.data(), timing_cuda.ipin_timing_data_rat, _t.ipin_timing_data_rat.size());
  memcpyD2HCUDA(irat2.data(), timing_cuda.ipint_timing_data_rat, _t.ipint_timing_data_rat.size());
  
  std::cout << "\n\n\nopin rat:\n";
  for (int i = 0; i < rat2.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
    //if (i == 0)
    //  std::cout << j << " " << _t.opin_string[j] << "\n";
    //if (i % 32 == 0 && i != 0) {
    if (i % 32 == 0) {
      //std::cout << std::endl;
      //std::cout << j << " " << _t.opin_string[j] << "\n";
      if (_t.opin_string[j] == "inst_0:ZN" || _t.opin_string[j] == "inst_3:ZN") {
          std::cout << _t.opin_string[j] << ": ";
          std::cout << rat2[i] << " ";
          std::cout << rat2[i+8] << " ";
          std::cout << rat2[i+16] << " ";
          std::cout << rat2[i+24] << "\n";
      }
    }
  }
  std::cout << "\n\n\n";
  
  std::cout << "ipin rat:\n";
  for (int i = 0; i < irat.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
    //if (i == 0)
    //  std::cout << j << " " << _t.opin_string[j] << "\n";
    //if (i % 32 == 0 && i != 0) {
    if (i % 32 == 0) { 
      if (_t.ipin_string[j] == "inst_3:A2" || _t.ipin_string[j] == "inst_2:A2") { 
          std::cout << _t.ipin_string[j] << ": " << i << " ";
          std::cout << irat[i] << " ";
          std::cout << irat[i+8] << " ";
          std::cout << irat[i+16] << " ";
          std::cout << irat[i+24] << "\n";
      }
    }
  }
  std::cout << "\n\n\n";
  
  std::cout << "ipint rat:\n";
  for (int i = 0; i < irat2.size(); ++i) {
    int ii = i % 32;
    int j = i / 32;
    if (i % 32 == 0) { 
      if (j == 8 || j == 9) { //for inst_14
          std::cout << "(" << i << ") ";
          std::cout << _t.ipin_string[j] << ": "; // not correct for ipin strings
          std::cout << irat2[i] << " ";
          std::cout << irat2[i+8] << " ";
          std::cout << irat2[i+16] << " ";
          std::cout << irat2[i+24] << "\n\n";
      }
    }
  }
  std::cout << "\n\n\n";
  */


  /*
  int l = _forward.size()-1;
  for (const auto& [level, insts] : _backward) {
    std::cout << "level: " << l-- << std::endl;
    for (const auto& ins : insts) {
      std::cout << ins << " ";
    }
    std::cout << "\n";
   
  }
  std::cout << "\n\n\n";
  */

  /*
  // 1) Propagate final output wire delays in reverse
    //    (assuming backward_levels.front() gives last level)
    auto& last = backward_levels.front();
    _addWireParamsForward(timing_cuda, rct_cuda,
                          num_inputs,
                          last.second,
                          last.first,
                          1);

    // 2) For each logic level in reverse topological order:
    for (auto& p : backward_levels) {
        int level    = p.first;
        int num_pins = p.second;
        _getTDataSense(timing_cuda, num_pins,
                       level, 1);
        _addDelayParam(timing_cuda, rct_cuda,
                       num_inputs,
                       num_pins,
                       level,
                       1);
        _max(timing_cuda,
             num_inputs,
             num_pins,
             level);
        _addWireParamsForward(timing_cuda, rct_cuda,
                              num_inputs,
                              num_pins,
                              level,
                              1);
    }
  */



  cudaError_t err = cudaDeviceSynchronize();
  if (err != cudaSuccess) {
    printf("CUDA Kernel Error: %s\n", cudaGetErrorString(err));
  }

  cudaEventRecord(stop);
  cudaEventSynchronize(stop);
  cudaEventElapsedTime(&prop, start, stop);
  //std::cout << "Propagation time: " << 2*prop << " ms" << std::endl;
  _copy_rct2cpu(rct_cuda);
  //_debug();

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