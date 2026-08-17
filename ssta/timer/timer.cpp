#include "timer.hpp"

Timer& Timer::read_celllib(std::filesystem::path path) 
{
  // Create a verilog module
  //auto celllib = std::make_shared<cell::Celllib>();
  //*celllib = cell::read_celllib(path);

  _celllib = std::make_shared<cell::Celllib>(cell::read_celllib(path));

  //_verilog(*module);

  return *this;
}

Timer& Timer::read_verilog(std::filesystem::path path) 
{
  // Create a verilog module
  auto module = std::make_shared<vlog::Module>();

  *module = vlog::read_verilog(path);

  _verilog(*module);

  return *this;
}

void Timer::_verilog(vlog::Module& module) 
{
  // Step 1: Scan the primary input and create a pin for each primary input port.
  // Each primary input is automatically connected to a net whose name is the same
  // as the name of the primary input.
  //for(const auto& pi : module.inputs) {
  //  _insert_primary_input(pi);
  //}
  
  // Step 2: Scan the primary output and create a pin for each primary output port.
  // Each primary output is automatically connected to a net whose name is the same
  // as the name of the primary output.
  //for(const auto& po : module.outputs) {
  //  _insert_primary_output(po);
  //}
  
  // Step 3: Scan the wires and insert a net for each wire. Each wire is automatically
  // attached to a net. The connection of each net is specified by the gates.
  //for(const auto& wire : module.wires) {
  //  _insert_net(wire);
  //}
  
  // Step 4: Scan the gate through verilog file. Insert a new gate for each gate
  // being iterated and a set of pins corresponding to each cellpin of the gate. Then,
  // for each pin-net mapping specified in the gate, connect the pin to the net.
  //for(const auto& gate : module.gates) {
    //_insert_gate(gate.name, gate.cell);
    //for(const auto& [c, n] : gate.cellpin2net) {
      //std::cout << gate.cell << " " << gate.name << ":" << c << std::endl;
      //auto& pin = _insert_pin(gate.name + ':' + c);
      //auto& net = _insert_net(n); 
      //_connect_pin(pin, net);
    //}
  //}
  _inputs = module.inputs;
  _outputs = module.outputs;

  
  // load cell lib
  if (!_celllib) 
    throw std::runtime_error("celllib not loaded!\n");
  auto cl = _celllib.value();

  for(const auto& gate : module.gates) {
    _gates.emplace_back(gate.name);
    _hasclk[gate.name] = cl->lib[gate.cell]->hasClkPin;
    _numipins[gate.name] = cl->lib[gate.cell]->numiPins;
    _numopins[gate.name] = cl->lib[gate.cell]->numoPins;
    _gate2cell[gate.name] = gate.cell;
    for(const auto& [c, n] : gate.cellpin2net) {
      //std::cout << "mapping: " << c << "->" << n << " " 
      //          << gate.name + ":" + c << " "
      //          << "input cell: " << gate.cell                
      //          << std::endl;
      std::string pin = gate.name + ":" + c; // inst_5:ZN 
      if (_pin2cell.find(pin) != _pin2cell.end())
        throw std::runtime_error("Duplicate pin to cell mapping\n");
      _pin2cell[pin] = gate.cell;
      if (std::find(cl->lib[gate.cell]->iPinNames.begin(), cl->lib[gate.cell]->iPinNames.end(), c) != cl->lib[gate.cell]->iPinNames.end()) {
        _ipin2gate[pin] = gate.name;
        _gate2ipin[gate.name].emplace_back(pin);
      }
      else if (c == cl->lib[gate.cell]->clkPinName) {
        _ipin2gate[pin] = gate.name;
        _gate2cpin[gate.name] = pin;
      }
      else {
        _opin2gate[pin] = gate.name;
        _gate2opin[gate.name].emplace_back(pin);
      }
      if (_hasclk[gate.name]) {
        if (c == cl->lib[gate.cell]->clkPinName) {
          _clknet[gate.name] = pin;
        }
      }
    }
  }

}


void Timer::_debug() {
  // DEBUG
  /*
  int num_net = _rct.names.size();
  for (int i = 0; i < num_net; ++i) {
    std::cout << std::endl;
    std::cout << i << std::endl;
    std::cout << "net: " << _rct.names[i] << std::endl;
    

    std::cout << "port: " << _rct.ports_name[i] << std::endl;
    int nodes_st = _rct.nodes_st[i];
    int nodes_en = _rct.nodes_st[i+1];
    int edges_st = _rct.edges_st[i];
    int edges_en = _rct.edges_st[i+1];
    int taps_st = _rct.taps_st[i];
    int taps_en = _rct.taps_st[i+1];

    
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _rct.caps[j] << " ";
    }
    std::cout << std::endl;
    std::cout << "\ndistances: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.distances[j] << " ";
    }
    std::cout << "\ncounts: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.counts[j] << " ";
    }
    std::cout << "\norders: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.orders[j] << " ";
    }
    std::cout << "\noutputs: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.outputs[j] << " ";
    }
    std::cout << "\npid: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.pid[j] << " ";
    }
    std::cout << "\npres: \n";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.pres[j] << " ";
    }
    std::cout << std::endl;
    
    for (int j = edges_st; j < edges_en; ++j) {
      std::cout << _rct.edges_s[j] << " ";
    }
    std::cout << std::endl;

    for (int j = edges_st; j < edges_en; ++j) {
      std::cout << _rct.edges_t[j] << " ";
    }
    std::cout << std::endl;
    
    for (int j = edges_st; j < edges_en; ++j) {
      std::cout << _deRct.orders[_rct.edges_s[j]] << " ";
    }
    std::cout << std::endl;

    for (int j = edges_st; j < edges_en; ++j) {
      std::cout << _deRct.orders[_rct.edges_t[j]] << " ";
    }
    std::cout << std::endl;


    
    std::cout << "taps: ";
    for (int j = taps_st; j < taps_en; ++j) {
      std::cout << _rct.taps_name[j] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "num nodes: " << _rct.nodes_st[i+1]-_rct.nodes_st[i] << std::endl;
    std::cout << "port: " << _rct.ports[i] << std::endl;
    int taps_st = _rct.taps_st[i];
    int taps_en = _rct.taps_st[i+1];
    std::cout << "tap names: ";
    for (int j = taps_st; j < taps_en; ++j) {
      std::cout << _rct.taps_name[j] << " ";
    }
    std::cout << "\ntaps: ";
    for (int j = taps_st; j < taps_en; ++j) {
      std::cout << _rct.taps[j] << " ";
    }
    std::cout << "\nedges: \n";
    for (int j = edges_st; j < edges_en; ++j) {
      std::cout << _rct.edges_s[j] << " -> " << _rct.edges_t[j] 
                << " : " << _rct.edges_s_str[j] << " -> " << _rct.edges_t_str[j]
                << " (" << _rct.ress[j] << ") " << std::endl;
    }
    
    

    //std::cout << "results:";
    
    
    std::cout << "\ndistances: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.distances[j] << " ";
    }
    std::cout << "\ncounts: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.counts[j] << " ";
    }
    std::cout << "\norders: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.orders[j] << " ";
    }
    
    
    std::cout << "caps: \n";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _rct.caps[j] << " ";
    }
    std::cout << "\npid: ";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.pid[j] << " ";
    }
    std::cout << "\npres: \n";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.pres[j] << " ";
    }
  
    

  
    
    std::cout << "delay: \n";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.delay[4*j] << " ";
                //<< _deRct.delay[4*j+1] << " " 
                //<< _deRct.delay[4*j+2] << " " 
                //<< _deRct.delay[4*j+3] << "\n";
    }
    std::cout << std::endl;

    std::cout << "slew: \n";
    for (int j = nodes_st; j < nodes_en; ++j) {
      std::cout << _deRct.slew[4*j] << " ";
                //<< _deRct.delay[4*j+1] << " " 
                //<< _deRct.delay[4*j+2] << " " 
                //<< _deRct.delay[4*j+3] << "\n";
    }
    std::cout << std::endl;
    

    std::cout << "tap cap: \n";
    for (int j = taps_st; j < taps_en; ++j) {
      std::cout << _rct.tapCaps[2*j] << " " << _rct.tapCaps[2*j + 1] << "\n"; 
    }
    
    std::cout << "tap delay: \n";
    for (int j = taps_st; j < taps_en; ++j) {
      std::cout << _deRct.tapDelay[4*j] << " " 
                << _deRct.tapDelay[4*j+1] << " " 
                << _deRct.tapDelay[4*j+2] << " " 
                << _deRct.tapDelay[4*j+3] << "\n";
    }
    
    std::cout << "tap slew: \n";
    for (int j = taps_st; j < taps_en; ++j) {
      std::cout << _deRct.tapSlew[4*j] << " " 
                << _deRct.tapSlew[4*j+1] << " " 
                << _deRct.tapSlew[4*j+2] << " " 
                << _deRct.tapSlew[4*j+3] << "\n";
    }
                
    //std::cout << std::endl;
  }
  */
  
}


void Timer::_build_connections(int num_threads) {

  _num_visited.clear();
  _num_fanout.clear();
  _vback.clear();
  
  for (const auto& gate : _gates) {
    _isvisited[gate] = false;
    _num_visited[gate] = 0;
  }

  for (const auto& pi : _inputs) {
    _q.push(pi);
  }

  int l = 0;
  while (!_q.empty()) {
    std::string pin = _q.front();
    _q.pop();
    if (_wires.find(pin) != _wires.end()) {
      for (const auto& tap : _wires[pin]) {
        if (_ipin2gate.find(tap) != _ipin2gate.end()) {
          std::string gate = _ipin2gate[tap];
          if (!_isvisited[gate]) {
            if (_hasclk[gate]) {
              if (tap == _clknet[gate]) {
                _isvisited[gate] = true;
                for (const auto& op : _gate2opin[gate]) {
                  _q2.push(op);
                }
                _forward[l].emplace_back(gate);
                _vback.emplace_back(gate);
              }
            }
            else {
              _num_visited[gate]++;
              if (_num_visited[gate] == _numipins[gate]) {
                _isvisited[gate] = true;
                for (const auto& op : _gate2opin[gate]) {
                  _q2.push(op); 
                  _num_fanout[gate] = _wires[op].size();
                }
                _forward[l].emplace_back(gate);
              }
            }
          }
        }
        else {
          // PO
        }
      }
    }
    if (_q.empty()) {
      //while (!q2.empty()) {
      //  q.push(q2.front());
      //  q2.pop();
      //}
      //l++;
      _q.swap(_q2);
      if (!_q.empty()) {
        l++;
      }
    }
  }

  // check
  //int total = 0;
  //for (const auto& [level, vs] : _forward) {
  //  total += vs.size();
  //}
  //assert(total == _gates.size());

  // backward
  for (const auto& gate : _gates) {
    _isvisited[gate] = false;
    _num_visited[gate] = 0;
  }
  for (const auto& po : _outputs) {
    _qback.push(po);
  }
  //for (const auto& v : _vback) {
  //  _backward[0].emplace_back(v);
  //  for (const auto& ipin : _gate2ipin[v]) {
  //    _qback.push(ipin);
  //  } 
  //}
  //l = 1;
  if (!_vback.empty()) {
    _backward[0].insert(_backward[0].end(), _vback.begin(), _vback.end());
    for (const auto& v : _vback) {
      for (const auto& ipin : _gate2ipin[v]) {
        _qback.push(ipin);
      } 
    }
  } 
  
  l = _vback.empty() ? 0 : 1; 
  while (!_qback.empty()) {
    std::string pin = _qback.front();
    _qback.pop();
    if (_rwires.find(pin) != _rwires.end()) {
      std::string port = _rwires[pin];
      if (_opin2gate.find(port) != _opin2gate.end()) {
        std::string gate = _opin2gate[port];
        if (!_isvisited[gate]) {
          _num_visited[gate]++;
          if (_num_visited[gate] == _num_fanout[gate]) {
            _isvisited[gate] = true;
            for (const auto& ip : _gate2ipin[gate]) {
              _q2.push(ip);
            }
            _backward[l].emplace_back(gate);
          }
        }
      }
    }
    if (_qback.empty()) {
      //while (!q2.empty()) {
      //  qback.push(q2.front());
      //  q2.pop();
      //}
      //l++;
      _qback.swap(_q2);
      if (!_qback.empty()) {
        l++;
      }
    }
  }

}


/*
void Timer::_build_connections(int num_threads) {
  omp_set_num_threads(num_threads);

  _num_visited.clear();
  _num_fanout.clear();
  _vback.clear();
  _forward.clear();
  _backward.clear();

  for (const auto& gate : _gates) {
    _isvisited[gate] = false;
    _num_visited[gate] = 0;
  }

  // =========================================================
  // Forward levelization (parallel within each level)
  // =========================================================
  
  // Seed: all primary inputs
  std::vector<std::string> current_frontier;
  for (const auto& pi : _inputs) {
    current_frontier.push_back(pi);
  }

  int l = 0;
  while (!current_frontier.empty()) {
    // For each pin in the current frontier, find all reachable gates
    // and collect next-level output pins
    //
    // We split this into two phases:
    //   Phase 1: (parallel) For each pin, find target gates and 
    //            atomically increment visit counts
    //   Phase 2: (parallel) Check which gates are now ready, 
    //            collect their output pins for the next frontier
    
    // Phase 1: Collect all (tap, gate) pairs from current frontier
    // Each frontier pin may fan out to multiple taps via wires
    struct TapInfo {
      std::string tap;
      std::string gate;
      bool is_clk_tap;
    };
    
    std::vector<TapInfo> all_taps;
    
    // This loop reads from wires/ipin2gate (no writes) — safe to parallelize reads
    // But std::unordered_map lookups are not thread-safe for concurrent reads 
    // with other writes, so we keep this serial for correctness
    // (the bottleneck is Phase 2 anyway)
    for (const auto& pin : current_frontier) {
      auto wit = _wires.find(pin);
      if (wit == _wires.end()) continue;
      
      for (const auto& tap : wit->second) {
        auto git = _ipin2gate.find(tap);
        if (git == _ipin2gate.end()) continue; // PO
        
        std::string gate = git->second;
        bool is_clk = false;
        if (_hasclk[gate] && tap == _clknet[gate]) {
          is_clk = true;
        }
        all_taps.push_back({tap, gate, is_clk});
      }
    }
    
    // Phase 2: Process taps and determine which gates are ready
    // We need atomicity for _num_visited increments
    std::vector<std::string> next_frontier;
    std::vector<std::string> level_gates;
    
    // Use a mutex for thread safety on shared containers
    // But since the main overhead is in large designs with many taps,
    // we can parallelize the increment and collect ready gates
    
    // For OpenMP: use atomic increments and collect in thread-local vectors
    int n_taps = all_taps.size();
    
    // Thread-local storage for gates and next pins
    std::vector<std::vector<std::string>> tl_level_gates(omp_get_max_threads());
    std::vector<std::vector<std::string>> tl_next_frontier(omp_get_max_threads());
    std::vector<std::vector<std::string>> tl_vback(omp_get_max_threads());
    
    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < n_taps; i++) {
      int tid = omp_get_thread_num();
      const auto& info = all_taps[i];
      const std::string& gate = info.gate;
      
      if (info.is_clk_tap) {
        // Clock pin: gate is ready immediately
        bool expected = false;
        // Use compare_exchange via a simple flag check with critical section
        bool should_process = false;
        #pragma omp critical(visit_check)
        {
          if (!_isvisited[gate]) {
            _isvisited[gate] = true;
            should_process = true;
          }
        }
        if (should_process) {
          for (const auto& op : _gate2opin[gate]) {
            tl_next_frontier[tid].push_back(op);
          }
          tl_level_gates[tid].push_back(gate);
          tl_vback[tid].push_back(gate);
        }
      }
      else {
        // Non-clock: increment visit count
        int count;
        #pragma omp atomic capture
        count = ++_num_visited[gate];
        
        if (count == _numipins[gate]) {
          #pragma omp critical(visit_check)
          {
            _isvisited[gate] = true;
          }
          for (const auto& op : _gate2opin[gate]) {
            tl_next_frontier[tid].push_back(op);
            // Compute fanout while we're here
            #pragma omp critical(fanout_update)
            {
              _num_fanout[gate] = _wires[op].size();
            }
          }
          tl_level_gates[tid].push_back(gate);
        }
      }
    }
    
    // Merge thread-local results
    for (int t = 0; t < omp_get_max_threads(); t++) {
      level_gates.insert(level_gates.end(), 
                         tl_level_gates[t].begin(), tl_level_gates[t].end());
      next_frontier.insert(next_frontier.end(), 
                           tl_next_frontier[t].begin(), tl_next_frontier[t].end());
      _vback.insert(_vback.end(), 
                     tl_vback[t].begin(), tl_vback[t].end());
    }
    
    if (!level_gates.empty()) {
      _forward[l] = std::move(level_gates);
      l++;
    }
    
    current_frontier = std::move(next_frontier);
  }

  // =========================================================
  // Backward levelization (parallel within each level)
  // =========================================================
  
  for (const auto& gate : _gates) {
    _isvisited[gate] = false;
    _num_visited[gate] = 0;
  }

  // Seed: primary outputs + vback (FF inputs)
  current_frontier.clear();
  for (const auto& po : _outputs) {
    current_frontier.push_back(po);
  }

  l = 0;
  if (!_vback.empty()) {
    _backward[0].insert(_backward[0].end(), _vback.begin(), _vback.end());
    for (const auto& v : _vback) {
      for (const auto& ipin : _gate2ipin[v]) {
        current_frontier.push_back(ipin);
      }
    }
    l = 1;
  }

  while (!current_frontier.empty()) {
    // Collect all (pin -> port -> gate) from current frontier
    struct BackInfo {
      std::string gate;
    };
    std::vector<BackInfo> all_gates;

    for (const auto& pin : current_frontier) {
      auto rit = _rwires.find(pin);
      if (rit == _rwires.end()) continue;
      
      std::string port = rit->second;
      auto oit = _opin2gate.find(port);
      if (oit == _opin2gate.end()) continue;
      
      all_gates.push_back({oit->second});
    }

    std::vector<std::string> next_frontier;
    std::vector<std::string> level_gates;

    int n_back = all_gates.size();
    std::vector<std::vector<std::string>> tl_level_gates(omp_get_max_threads());
    std::vector<std::vector<std::string>> tl_next_frontier(omp_get_max_threads());

    #pragma omp parallel for schedule(dynamic, 64)
    for (int i = 0; i < n_back; i++) {
      int tid = omp_get_thread_num();
      const std::string& gate = all_gates[i].gate;

      int count;
      #pragma omp atomic capture
      count = ++_num_visited[gate];

      if (count == _num_fanout[gate]) {
        #pragma omp critical(back_visit)
        {
          _isvisited[gate] = true;
        }
        for (const auto& ip : _gate2ipin[gate]) {
          tl_next_frontier[tid].push_back(ip);
        }
        tl_level_gates[tid].push_back(gate);
      }
    }

    // Merge
    for (int t = 0; t < omp_get_max_threads(); t++) {
      level_gates.insert(level_gates.end(),
                         tl_level_gates[t].begin(), tl_level_gates[t].end());
      next_frontier.insert(next_frontier.end(),
                           tl_next_frontier[t].begin(), tl_next_frontier[t].end());
    }

    if (!level_gates.empty()) {
      _backward[l] = std::move(level_gates);
      l++;
    }

    current_frontier = std::move(next_frontier);
  }
}
*/

void Timer::_get_arch_timing() {
  // load cell lib
  if (!_celllib) 
    throw std::runtime_error("celllib not loaded!\n");
  auto cl = _celllib.value();
  
  // mapping
  std::unordered_map<std::string, int> tcell_map;
  std::unordered_map<std::string, int> port_map;
  for (int i = 0; i < _rct.ports_name.size(); ++i) {
    port_map[_rct.ports_name[i]] = i;
  }

  // convert the used cell lib to vector
  int cell_index = 0;
  int cell_timing_ptr = 0; // exclusive scan of num output pin
  int cell_constraint_ptr = 0; // exclusive scan of num input pin
  int cell_arch_ptr = 0; // exclusive scan of (num input pin)*(num output pin)
  int ipin_index = 0;
  int opin_index = 0;
  std::unordered_map<std::string, int> cell_map; // give a cell return the index in the vector
  std::unordered_map<std::string, int> ipin_map; // map id to each ipin name
  std::unordered_map<std::string, int> opin_map; // map id to each opin name
  std::unordered_map<std::string, int> lib_opin_index;
  std::unordered_map<std::string, int> map2forward;
  std::unordered_map<std::string, int> backward_nipin_ptr;
  for (const auto& input : _inputs) {
    if (ipin_map.find(input) == ipin_map.end()) {
      ipin_map[input] = ipin_index++;
    }
  }
  for (const auto& gate : _gates) { // TODO: check _gates to _forward
    //std::string cell = _gate2cell[gate];
    std::string cell = _gate2cell.at(gate);
    if (cell_map.find(cell) == cell_map.end()) {
      cell_map[cell] = cell_index;
      cell_index++;
      if (_hasclk[gate]) { // for FF
        // timing
        //_t.tcell_numIPins.emplace_back(cl->lib[cell]->numiPins);
        //_t.tcell_numOPins.emplace_back(cl->lib[cell]->numoPins);
        for (int o = 0; o < cl->lib[cell]->numoPins; ++o) {
          std::string oP = cl->lib[cell]->oPinNames[o];
          _t.tcell_timing_sense.emplace_back(cl->lib[cell]->coTiming[oP].sense); // P, Neg, Non
          for (int ii = 0; ii < 9; ++ii) {
            _t.tcell_timing_data_slew.emplace_back(cl->lib[cell]->coTiming[oP].fallSlew[ii]);
          }
          for (int ii = 0; ii < 9; ++ii) {
            _t.tcell_timing_data_slew.emplace_back(cl->lib[cell]->coTiming[oP].riseSlew[ii]);
          }
          for (int ii = 0; ii < 9; ++ii) {
            _t.tcell_timing_data_delay.emplace_back(cl->lib[cell]->coTiming[oP].fallDelay[ii]);
          }
          for (int ii = 0; ii < 9; ++ii) {
            _t.tcell_timing_data_delay.emplace_back(cl->lib[cell]->coTiming[oP].riseDelay[ii]);
          }
          lib_opin_index[cell+":"+oP] = o;
        }
        // constraint
        for (int i = 0; i < cl->lib[cell]->numiPins; ++i) {
          std::string iP = cl->lib[cell]->iPinNames[i];
          //_t.tcell_constraint_setup_type.emplace_back(cl->lib[cell]->setup[iP].edgeType);  // Falling, Rising
          for (int ii = 0; ii < 3; ++ii) {
            _t.tcell_constraint_setup_data.emplace_back(cl->lib[cell]->setup[iP].fall[ii]);
          }
          for (int ii = 0; ii < 3; ++ii) {
            _t.tcell_constraint_setup_data.emplace_back(cl->lib[cell]->setup[iP].rise[ii]);
          }
          _t.tcell_constraint_hold_type.emplace_back(cl->lib[cell]->hold[iP].edgeType);
          for (int ii = 0; ii < 3; ++ii) {
            _t.tcell_constraint_hold_data.emplace_back(cl->lib[cell]->hold[iP].fall[ii]);
          }
          for (int ii = 0; ii < 3; ++ii) {
            _t.tcell_constraint_hold_data.emplace_back(cl->lib[cell]->hold[iP].rise[ii]);
          }
        }

      }
      else { // for combinational cell
        //_t.cell_numIPins.emplace_back(cl->lib[cell]->numiPins);
        //_t.cell_numOPins.emplace_back(cl->lib[cell]->numoPins);
        for (int o = 0; o < cl->lib[cell]->numoPins; ++o) {
          std::string oP = cl->lib[cell]->oPinNames[o];
          for (int i = 0; i < cl->lib[cell]->numiPins; ++i) {
            std::string iP = cl->lib[cell]->iPinNames[i];
            _t.cell_arch_sense.emplace_back(cl->lib[cell]->ioTiming[iP][oP].sense); // P, Neg, Non
            for (int ii = 0; ii < 9; ++ii) {
              _t.cell_arch_data_slew.emplace_back(cl->lib[cell]->ioTiming[iP][oP].fallSlew[ii]);
            }
            for (int ii = 0; ii < 9; ++ii) {
              _t.cell_arch_data_slew.emplace_back(cl->lib[cell]->ioTiming[iP][oP].riseSlew[ii]);
            }
            for (int ii = 0; ii < 9; ++ii) {
              _t.cell_arch_data_delay.emplace_back(cl->lib[cell]->ioTiming[iP][oP].fallDelay[ii]);
            }
            for (int ii = 0; ii < 9; ++ii) {
              _t.cell_arch_data_delay.emplace_back(cl->lib[cell]->ioTiming[iP][oP].riseDelay[ii]);
            }
          }
          lib_opin_index[cell+":"+oP] = o;
        }
      }
      _t.tcell_timing_ptr.emplace_back(cell_timing_ptr);
      _t.tcell_constraint_ptr.emplace_back(cell_constraint_ptr);
      _t.cell_arch_ptr.emplace_back(cell_arch_ptr);
      if (_hasclk[gate]) {
        cell_timing_ptr += cl->lib[cell]->numoPins;
        cell_constraint_ptr += cl->lib[cell]->numiPins;
      }
      else {
        cell_arch_ptr += (cl->lib[cell]->numiPins) * (cl->lib[cell]->numoPins);
      }
    }


    // io pin
    if (_hasclk[gate]) {
      // cpin
      std::string cpin = _gate2cpin[gate];
      if (ipin_map.find(cpin) == ipin_map.end()) {
        ipin_map[cpin] = ipin_index++;
      }
    }
    auto it = _gate2ipin.find(gate);
    if (it == _gate2ipin.end()) { // gate not found—handle error or return early
      std::cerr << "Error: Gate " << gate << " not found in _gate2ipin.\n";
      return;
    }
    auto &pins = it->second;        // safe: no unintended insertion
    std::sort(pins.begin(), pins.end());
    for (const auto& ipin : _gate2ipin[gate]) {
      if (ipin_map.find(ipin) == ipin_map.end()) {
        ipin_map[ipin] = ipin_index++;
      }
    }
    for (const auto& opin : _gate2opin[gate]) {
      if (opin_map.find(opin) == opin_map.end()) {
        opin_map[opin] = opin_index++;
      }
    }
  }
  _t.tcell_timing_ptr.emplace_back(cell_timing_ptr);
  _t.tcell_constraint_ptr.emplace_back(cell_constraint_ptr);
  _t.cell_arch_ptr.emplace_back(cell_arch_ptr);
  
  for (const auto& out : _outputs) {
    if (ipin_map.find(out) == ipin_map.end()) {
      ipin_map[out] = ipin_index++;
    }
  }


  // gate cell type to load lib data -> cell_type
  // gate has clk or not -> cell_hasclk
  // gate input pins id -> ipin
  // gate input pins (from where to where) -> ipin_ptr
  // gate total input pins per level -> ipin_level_ptr
  // gate output pins id -> opin
  // gate input pins (from where to where) -> opin_ptr
  // gate total output pins per level -> opin_level_ptr
  // gate output pin's wire port -> wp
  // gate output pin's wire port to tap to next level gate input to store data -> nipin
  int num_ipin = 0;
  int num_opin = 0;
  int num_gates = 0;
  int num_nipin = 0;
  //_t.ipin_ptr.emplace_back(num_ipin);
  _t.opin_ptr.emplace_back(num_opin);
  _t.ipint_ptr.emplace_back(0);
  _t.nipin_ptr.emplace_back(num_nipin);
  _t.ipin_level_ptr.emplace_back(num_ipin); // don't need this
  _t.opin_level_ptr.emplace_back(num_opin);
  _t.nipin_level_ptr.emplace_back(num_nipin);
  _t.cell_level_ptr.emplace_back(num_gates);
  for (const auto& port : _inputs) {
    if (ipin_map.find(port) != ipin_map.end()) {
      _t.ipin.emplace_back(ipin_map[port]);
      //_t.ipin_string.emplace_back(port);
      _t.ipin_ptr.emplace_back(num_ipin++);
    }
    //_t.opin.emplace_back(opin_map[port]);
    //_t.opin_string.emplace_back(port);
    // port --> net
    // net --> tap
    _t.wp.emplace_back(port_map[port]);
    for (const auto& taps : _wires[port]) {
      if (ipin_map.find(taps) != ipin_map.end()) {
        _t.nipin.emplace_back(ipin_map[taps]); // this means the next ipin
        _t.nipin_string.emplace_back(taps);
        num_nipin++;
      }
    }
    _t.nipin_ptr.emplace_back(num_nipin);
    //num_nipin += _wires[port].size();
  }
  _t.ipin_ptr.emplace_back(num_ipin);

  /*
  for (const auto& wp : _t.wp) {
    std::cout << wp << " ";
    std::cout << " : " << _rct.ports_name[wp] << " -> ";
    for (int j = _rct.taps_st[wp]; j < _rct.taps_st[wp+1]; ++j) {
      std::cout << _rct.taps_name[j] << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  */
  
  //num_gates += _inputs.size();
  //_t.ipin_level_ptr.emplace_back(num_ipin);
  _t.nipin_level_ptr.emplace_back(num_nipin);
  //_t.cell_level_ptr.emplace_back(num_gates);

  int num_of_ipins = 0;
  int num_of_opins = 0;
  int num_of_archs = 0;

  int print = 0;
  int map2f_index = 0;

  for (const auto& [level, insts] : _forward) {
    if (print) std::cout << "\n------------\n\n\nlevel: " << level << std::endl;
    for (const auto& gate : insts) {
      map2forward[gate] = map2f_index++;
      if (print) std::cout << gate << ": \n";
      std::string cell = _gate2cell[gate];
      _t.cell_type.emplace_back(cell_map[cell]);
      // input pins
      if (_hasclk[gate]) {
        _t.cell_hasclk.emplace_back(1);
        std::string cpin = _gate2cpin[gate];
        if (print) std::cout << " cpin: " << cpin << "\n";
        if (ipin_map.find(cpin) != ipin_map.end()) {
          _t.ipin.emplace_back(ipin_map[cpin]);
          num_ipin++;
          num_of_ipins++;
          //_t.ipin_ptr.emplace_back(num_ipin);
        }
      }
      else {
        _t.cell_hasclk.emplace_back(0);
      }
      if (print) std::cout << " ipin: ";
      for (const auto& ipin : _gate2ipin[gate]) {
        if (print) std::cout << ipin << " ";
        if (ipin_map.find(ipin) != ipin_map.end()) {
          _t.ipin.emplace_back(ipin_map[ipin]);
          num_ipin++;
          num_of_ipins++;
        }
      }
      if (print) std::cout << "\n";
      //num_ipin += _gate2ipin[gate].size();
      _t.ipin_ptr.emplace_back(num_ipin);

      // output pin
      if (print) std::cout << " opin: ";
      for (const auto& opin : _gate2opin[gate]) {
        if (print) std::cout << opin << " -> ";
        _t.opin.emplace_back(num_opin++);
        num_of_opins++;
        _t.opin_string.emplace_back(opin);
        _t.wp.emplace_back(port_map[opin]);
        backward_nipin_ptr[opin] = num_nipin;
        for (const auto& taps : _wires[opin]) {
          try {
            _t.nipin.emplace_back(ipin_map.at(taps)); // this means the next ipin
            _t.nipin_string.emplace_back(taps);
            num_nipin++;
            if (print) std::cout << taps << " (" << ipin_map[taps] << ") ";
          }
          catch (const std::out_of_range& e) {
            std::cerr << "Error: tap " << taps << " not found in ipin_map!\n";
            throw;
          }
        }
        if (print) std::cout << "\n";
        _t.nipin_ptr.emplace_back(num_nipin);
        std::string c = _gate2cell.at(gate);
        size_t pos = opin.find(':');
        std::string suffix;
        if (pos != std::string::npos) {
          suffix = opin.substr(pos + 1);  // everything after ':'
        }
        std::string opin_name = c + ":" + suffix;
        try {
          _t.opin_index.emplace_back(lib_opin_index.at(opin_name));
        }
        catch (const std::out_of_range& e) {
          std::cerr << "Error: Key '" << opin_name << "' not found in lib_opin_index.\n";
          for (const auto& [k, v] : lib_opin_index) {
            std::cerr << k << " : " << v << "\n";
          }
          throw; // rethrow the exception after logging
        }
        
      }
      if (print) std::cout << "\n";
      _t.opin_ptr.emplace_back(num_opin);
      num_of_archs += num_of_ipins * num_of_opins;
      num_of_ipins = 0;
      num_of_opins = 0;
      _t.ipint_ptr.emplace_back(num_of_archs);
    }
    num_gates += insts.size();
    _t.ipin_level_ptr.emplace_back(num_ipin);
    _t.nipin_level_ptr.emplace_back(num_nipin);
    _t.cell_level_ptr.emplace_back(num_gates);
    _t.opin_level_ptr.emplace_back(num_opin);
  }
  //_t.opin_ptr.emplace_back(num_opin);
  //_t.nipin_ptr.emplace_back(num_nipin);
  for (const auto& out : _outputs) {
    if (ipin_map.find(out) != ipin_map.end()) {
      _t.ipin.emplace_back(ipin_map[out]);
      _t.outputs_ipin.emplace_back(ipin_map[out]);
      num_ipin++;
      num_of_ipins++;
    }
  }
  _t.ipin_level_ptr.emplace_back(num_ipin);

  // this is a very important remapping process
  _t.ipin_string.resize(ipin_map.size());
  for (const auto& [k, v] : ipin_map) {
    _t.ipin_string[v] = k;
  }

  for (const auto& opin : _outputs) {
    _t.output_string.emplace_back(opin);
  }


  // I need a remapping to forward
  num_gates = 0;
  num_opin = 0;
  _t.cell_blevel_ptr.emplace_back(0);
  _t.bnipin_ptr_each_gate.emplace_back(0);
  for (const auto& [level, insts] : _backward) {
    //std::cout << "level: " << level << std::endl;
    for (const auto& inst : insts) {
      //std::cout << inst << " ";
      _t.map_to_forward.emplace_back(map2forward[inst]);
      for (const auto& opin : _gate2opin[inst]) {
        _t.bnipin_ptr.emplace_back(backward_nipin_ptr[opin]);
      }
      num_opin += _gate2opin[inst].size();
      _t.bnipin_ptr_each_gate.emplace_back(num_opin);
    }
    //std::cout << std::endl;
    num_gates += insts.size();
    _t.cell_blevel_ptr.emplace_back(num_gates);
  }

  /*
  std::cout << "bnipin_ptr_each_gate: ";
  for (const auto& p : _t.bnipin_ptr_each_gate) {
    std::cout << p << " ";
  }
  std::cout << std::endl;
  std::cout << "bnipin_ptr: ";
  for (const auto& p : _t.bnipin_ptr) {
    std::cout << p << " ";
  }
  std::cout << std::endl;
  std::cout << "nipin: ";
  for (const auto& p : _t.nipin) {
    std::cout << p << " ";
  }
  std::cout << std::endl;
  */


  // debug
  // (1) cell_level_ptr: OK
  // (2) cell_hasclk; OK
  // (3) cell_type; OK
  // (4) ipin;
  // (5) ipin_ptr;
  // (6) ipin_ptr_level;
  // (7) wp;
  // (8) nipins
  // (9) nipin_ptr;
  // (10) nipin_level_ptr;

  //std::cout << "_t.ipin.size(): " << _t.ipin.size() << std::endl;
  //std::cout << "_t.opin.size(): " << _t.opin.size() << std::endl;

  
  // timing
  _t.ipin_timing_data_slew.resize(_t.ipin.size()*32);
  _t.ipin_timing_data_at.resize(_t.ipin.size()*32);
  _t.ipin_timing_data_rat.resize(_t.ipin.size()*32);
  for (const auto& port : _inputs) {
    int start = ipin_map[port];
    for (int j = 0; j < 4; ++j) {
      _t.ipin_timing_data_slew[32*start + j*8] = _slew[port][j];
      _t.ipin_timing_data_at[32*start + j*8] = _at[port][j];
    }
  }
  for (int i = 0; i < _outputs.size(); ++i) {
    int start = ipin_map[_outputs[i]];
    for (int j = 0; j < 4; ++j) {
      _t.ipin_timing_data_rat[32*start + j*8] = _rat[_outputs[i]][j];
    }
  }
  _t.opin_timing_data_slew.resize(_t.opin.size()*32);
  _t.opin_timing_data_at.resize(_t.opin.size()*32);
  _t.opin_timing_data_rat.resize(_t.opin.size()*32);

  //temp
  _t.ipint_timing_data_slew.resize(num_of_archs*32);
  _t.ipint_timing_data_at.resize(num_of_archs*32);
  _t.ipint_timing_data_rat.resize(num_of_archs*32);

}

// Build canonical per-cell pin orders once (per cell type) using LIB order
CanonPins Timer::build_canon_pins(const cell::Cell* cellp) {
  CanonPins cp;

  // MUST match the packing loops that use iPinNames/oPinNames
  cp.ipins.reserve(cellp->numiPins);
  cp.opins.reserve(cellp->numoPins);

  for (int ii = 0; ii < cellp->numiPins; ++ii) {
    cp.ipins.push_back(cellp->iPinNames[ii]);   // LIB order
  }
  for (int oo = 0; oo < cellp->numoPins; ++oo) {
    cp.opins.push_back(cellp->oPinNames[oo]);   // LIB order
  }

  cp.i_idx.reserve(cp.ipins.size());
  for (int i = 0; i < (int)cp.ipins.size(); ++i) cp.i_idx[cp.ipins[i]] = i;

  cp.o_idx.reserve(cp.opins.size());
  for (int o = 0; o < (int)cp.opins.size(); ++o) cp.o_idx[cp.opins[o]] = o;

  return cp;
}



PinGraph Timer::export_levelized_pin_deps() const {

  if (!_celllib) {
    throw std::runtime_error("export_levelized_pin_deps: celllib not loaded");
  }

  PinGraph G;

  // Local canonical pin maps aligned by cell_uid (avoid mutating Timer in const method)
  std::vector<CanonPins> canon_by_uid;

  // -------------------------
  // Helpers
  // -------------------------
  auto get_pin_id = [&](const std::string& p) -> int {
    auto it = G.pin2id.find(p);
    if (it != G.pin2id.end()) return it->second;
    int id = (int)G.id2pin.size();
    G.pin2id.emplace(p, id);
    G.id2pin.emplace_back(p);
    return id;
  };

  auto get_gate_id = [&](const std::string& g) -> int {
    auto it = G.gate2id.find(g);
    if (it != G.gate2id.end()) return it->second;
    int id = (int)G.id2gate.size();
    G.gate2id.emplace(g, id);
    G.id2gate.emplace_back(g);
    return id;
  };

  auto local_pin = [](const std::string& instpin) -> std::string {
    auto pos = instpin.find(':');
    if (pos == std::string::npos) return instpin;
    return instpin.substr(pos + 1);
  };

  // -------------------------
  // 1) Build pin universe (IDs)
  // -------------------------
  for (const auto& p : _inputs)  G.pi_ids.emplace_back(get_pin_id(p));
  for (const auto& p : _outputs) G.po_ids.emplace_back(get_pin_id(p));

  for (const auto& kv : _pin2cell) {
    get_pin_id(kv.first); // instance pin "inst:PIN"
  }

  // -------------------------
  // 2) Build gate universe (IDs)
  // -------------------------
  for (const auto& g : _gates) {
    get_gate_id(g);
  }

  // -------------------------
  // 3) Flatten forward levels
  //    _forward is std::map<int, vector<string>>
  // -------------------------
  const int L = (int)_forward.size();
  G.level_ptr.clear();
  G.level_gate_ids.clear();
  G.level_ptr.reserve(L + 1);
  G.level_ptr.emplace_back(0);

  // Iterate levels in increasing key order (std::map already sorted)
  for (const auto& [lvl, gates] : _forward) {
    (void)lvl;
    for (const auto& gname : gates) {
      G.level_gate_ids.emplace_back(get_gate_id(gname));
    }
    G.level_ptr.emplace_back((int)G.level_gate_ids.size());
  }

  // -------------------------
  // 4) Export per-level edges
  // -------------------------
  auto cl = _celllib.value();

  std::unordered_map<std::string, int> cell2uid;
  cell2uid.reserve(_gates.size());
  int next_uid = 0;

  auto get_cell_uid = [&](const std::string& cellname) -> int {
    auto it = cell2uid.find(cellname);
    if (it != cell2uid.end()) return it->second;

    int uid = next_uid++;
    cell2uid.emplace(cellname, uid);

    // record this cell's dimensions
    auto it_lib = cl->lib.find(cellname);
    if (it_lib == cl->lib.end()) {
      throw std::runtime_error("cell not found in lib: " + cellname);
    }

    const cell::Cell* c = it_lib->second.get();
    // build canonical pins for this uid (LIB order!)
    if ((int)canon_by_uid.size() <= uid) canon_by_uid.resize(uid + 1);
    canon_by_uid[uid] = build_canon_pins(c);

    G.cell_numi.emplace_back(c->numiPins);
    G.cell_numo.emplace_back(c->numoPins);

    //std::cout << cellname << " " << uid << " is seq?: " << c->coTiming.empty() << std::endl;
    bool is_seq = !c->coTiming.empty();
    G.cell_is_seq.emplace_back((uint8_t)(is_seq ? 1 : 0));

    // After you compute uid and have `cell::Cell* c` and `bool is_seq`
    if ((int)G.cell_arch_ptr.size() <= uid) G.cell_arch_ptr.resize(uid + 1, 0);
    if ((int)G.tcell_ptr.size()     <= uid) G.tcell_ptr.resize(uid + 1, 0);

    // Always define BOTH pointers for this uid
    G.cell_arch_ptr[uid] = (int)G.cell_arch_sense.size();
    G.tcell_ptr[uid]     = (int)G.tcell_sense.size();

    if (!is_seq) { // comb: reserve ptr to total (i, o) pairs
      //int base_ptr = (int)G.cell_arch_sense.size();
      //G.cell_arch_ptr.emplace_back(base_ptr);
      for (int ii = 0; ii < c->numiPins; ++ii) { // append all (i,o) pairs in LIB order
        const std::string& iP = c->iPinNames[ii];
        auto it_i = c->ioTiming.find(iP);

        for (int oo = 0; oo < c->numoPins; ++oo) {
          const std::string& oP = c->oPinNames[oo];

          // if missing, append zero to keep it dense
          cell::Timing_Data arc{};
          bool has = false;
          if (it_i != c->ioTiming.end()) {
            auto it_o = it_i->second.find(oP);
            if (it_o != it_i->second.end() && it_o->second.hasData) {
              arc = it_o->second;
              has = true;
            }
          }

          // sense
          G.cell_arch_sense.emplace_back((uint8_t)(has ? arc.sense : cell::TimingSense::PositiveUnate));

          // delay 18 floats: fall[0..8], rise[0..8]
          for (int k = 0; k < 9; ++k) G.cell_arch_delay.emplace_back(has ? arc.fallDelay[k] : 0.0f);
          for (int k = 0; k < 9; ++k) G.cell_arch_delay.emplace_back(has ? arc.riseDelay[k] : 0.0f);

          // slew 18 floats
          for (int k = 0; k < 9; ++k) G.cell_arch_slew.emplace_back(has ? arc.fallSlew[k] : 0.0f);
          for (int k = 0; k < 9; ++k) G.cell_arch_slew.emplace_back(has ? arc.riseSlew[k] : 0.0f);
        } // end oo
      } // end ii
    } 
    else { // seq: CK->Q per output pin only
      //int base_o = (int)G.tcell_sense.size();
      //G.tcell_ptr.emplace_back(base_o);

      for (int oo = 0; oo < c->numoPins; ++oo) {
        const std::string& oP = c->oPinNames[oo];
        auto itq = c->coTiming.find(oP);
        bool has = (itq != c->coTiming.end());
        //const auto& arc = itq->second;
        //G.tcell_sense.emplace_back((uint8_t)(has ? arc.sense : cell::TimingSense::PositiveUnate));
        
        G.tcell_sense.emplace_back((uint8_t)(has ? itq->second.sense : cell::TimingSense::PositiveUnate));

        for (int k = 0; k < 9; ++k) G.tcell_delay.emplace_back(has ? itq->second.fallDelay[k] : 0.0f);
        for (int k = 0; k < 9; ++k) G.tcell_delay.emplace_back(has ? itq->second.riseDelay[k] : 0.0f);
        for (int k = 0; k < 9; ++k) G.tcell_slew.emplace_back(has ? itq->second.fallSlew[k] : 0.0f);
        for (int k = 0; k < 9; ++k) G.tcell_slew.emplace_back(has ? itq->second.riseSlew[k] : 0.0f);
      }

      // setup_ptr[uid] points to where this cell's setup data starts
      if ((int)G.setup_ptr.size() <= uid) G.setup_ptr.resize(uid + 1, 0);
      if ((int)G.hold_ptr.size()  <= uid) G.hold_ptr.resize(uid + 1, 0);
      G.setup_ptr[uid] = (int)G.setup_data.size();
      G.hold_ptr[uid]  = (int)G.hold_data.size();

      for (int ii = 0; ii < c->numiPins; ++ii) {
        const std::string& iP = c->iPinNames[ii];  // ← use the pin NAME

        auto it_su = c->setup.find(iP);   // ← string key, not int
        if (it_su != c->setup.end()) {
          for (int k = 0; k < 3; ++k) G.setup_data.emplace_back(it_su->second.rise[k]);
          for (int k = 0; k < 3; ++k) G.setup_data.emplace_back(it_su->second.fall[k]);
        } else {
          for (int k = 0; k < 6; ++k) G.setup_data.emplace_back(0.0f);
        }

        auto it_ho = c->hold.find(iP);    // ← string key, not int
        if (it_ho != c->hold.end()) {
          for (int k = 0; k < 3; ++k) G.hold_data.emplace_back(it_ho->second.rise[k]);
          for (int k = 0; k < 3; ++k) G.hold_data.emplace_back(it_ho->second.fall[k]);
        } else {
          for (int k = 0; k < 6; ++k) G.hold_data.emplace_back(0.0f);
        }
      }

    }

    return uid;
  };
  

  G.net_level_ptr.clear();
  G.arc_level_ptr.clear();
  G.net_src.clear(); G.net_dst.clear(); G.net_tap_idx.clear();
  G.arc_src.clear(); G.arc_dst.clear();
  G.net_level_ptr.reserve(L + 1);
  G.arc_level_ptr.reserve(L + 1);

  G.net_level_ptr.emplace_back(0);
  G.arc_level_ptr.emplace_back(0);

  // We’ll dedup edges per level (avoid duplicates due to repeated taps/arc entries).
  // Use a 64-bit key (src<<32 | dst).
  auto pack_edge = [](int u, int v) -> uint64_t {
    return (uint64_t(uint32_t(u)) << 32) | uint64_t(uint32_t(v));
  };

  G.tap_id2pin  = _rct.taps_name;  // same indexing as tapDelay/tapSlew
  G.port_id2pin = _rct.ports_name; // same indexing as ports 

  // PI net edges: PI port -> sink gate ipin
  for (const auto& pi : _inputs) {
    auto itw = _wires.find(pi);
    if (itw == _wires.end()) continue;

    int u = get_pin_id(pi);
    for (const auto& tap : itw->second) {
      bool is_gate_ipin = _ipin2gate.count(tap);
      if (!is_gate_ipin) continue;

      int v = get_pin_id(tap);
      uint64_t key = pack_edge(u, v);

      auto itTap = _rct.tap2idx.find(tap);
      int t = (itTap == _rct.tap2idx.end()) ? -1 : itTap->second;

      // you can put these into level 0 net edges or a separate list
      G.net_src.emplace_back(u);
      G.net_dst.emplace_back(v);
      G.net_tap_idx.emplace_back(t);
    }
  }
  

  std::unordered_set<std::string> po_set(_outputs.begin(), _outputs.end());
  std::unordered_set<std::string> pi_set(_inputs.begin(), _inputs.end());
  std::vector<std::tuple<int,int,int,int>> ff_pin_info; // (pin_id, clk_pin, uid, ilocal)
  for (int li = 0; li < L; li++) {
    std::unordered_set<uint64_t> net_seen;
    std::unordered_set<uint64_t> arc_seen;

    // gates in this level are in [level_ptr[li], level_ptr[li+1])
    int gb = G.level_ptr[li];
    int ge = G.level_ptr[li + 1];

    for (int k = gb; k < ge; k++) {
      int gid = G.level_gate_ids[k];
      const std::string& gname = G.id2gate[gid];

      // ---- 4A) NET edges: gate output pins -> sink input pins (fanout) ----
      auto it_opins = _gate2opin.find(gname);
      if (it_opins != _gate2opin.end()) {
        for (const auto& op : it_opins->second) {
          auto itw = _wires.find(op);
          if (itw == _wires.end()) continue;

          int u = get_pin_id(op);

          for (const auto& tap : itw->second) {
            // connect to either a gate input pin OR a primary output port
            bool is_gate_ipin = _ipin2gate.count(tap);
            bool is_po = po_set.count(tap);
            
            if (!(is_gate_ipin || is_po)) continue;

            int v = get_pin_id(tap);

            uint64_t key = pack_edge(u, v);
            if (net_seen.insert(key).second) {
              int t = -1;
              
              auto itTap = _rct.tap2idx.find(tap);
              if (itTap != _rct.tap2idx.end()) t = itTap->second;

              
              G.net_src.emplace_back(u);
              G.net_dst.emplace_back(v);
              G.net_tap_idx.emplace_back(t);
            }
          }
        }
      }

      // ---- 4B) ARC edges: gate input pins -> gate output pins (cell arcs) ----
      auto it_cell = _gate2cell.find(gname);
      if (it_cell == _gate2cell.end()) continue;

      const std::string& cellname = it_cell->second;
      auto it_lib = cl->lib.find(cellname);
      if (it_lib == cl->lib.end()) continue;

      const cell::Cell* cellp = it_lib->second.get();

      auto it_ipins2 = _gate2ipin.find(gname);
      auto it_opins2 = _gate2opin.find(gname);
      if (it_ipins2 == _gate2ipin.end() || it_opins2 == _gate2opin.end()) continue;

      const auto& ipins = it_ipins2->second;
      const auto& opins = it_opins2->second;

      // ---- 4B0) SEQ arcs: CK -> Q/QN (red) ----
      bool has_clk = false;
      if (auto it = _hasclk.find(gname); it != _hasclk.end()) {
        has_clk = it->second;
      }
      if (has_clk) {
        int cell_uid = get_cell_uid(cellname);
        const auto& canon = canon_by_uid[cell_uid];

        auto it_ck = _gate2cpin.find(gname);
        if (it_ck != _gate2cpin.end()) {
          int u_ck = get_pin_id(it_ck->second);

          for (const auto& qpin : opins) {
            int v_q = get_pin_id(qpin);
            uint64_t key = pack_edge(u_ck, v_q);
            if (arc_seen.insert(key).second) {
              G.arc_src.emplace_back(u_ck);
              G.arc_dst.emplace_back(v_q);

              G.arc_kind.emplace_back(uint8_t{1});
              G.arc_cell_uid.emplace_back(cell_uid);
              G.arc_i_local.emplace_back(0);                 // unused
              std::string oq = local_pin(qpin);
              auto itO = canon.o_idx.find(oq);
              if (itO == canon.o_idx.end()) {
                throw std::runtime_error("SEQ arc output pin not in canon: cell=" + cellname + " pin=" + oq);
              }
              G.arc_o_local.emplace_back(itO->second);
              G.arc_clk_pin.emplace_back(u_ck);
            }
          }

          for (const auto& ipin : ipins) {
            int ipin_id = get_pin_id(ipin);
            std::string local = local_pin(ipin);
            auto itI = canon.i_idx.find(local);
            int iloc = (itI != canon.i_idx.end()) ? itI->second : 0;
            ff_pin_info.emplace_back(ipin_id, u_ck, cell_uid, iloc);
          }
        }

        continue;
      }

      int cell_uid = get_cell_uid(cellname);
      const auto& canon = canon_by_uid[cell_uid];

      for (const auto& ipin : ipins) {
        std::string i = local_pin(ipin);
        auto it_i = cellp->ioTiming.find(i);
        if (it_i == cellp->ioTiming.end()) continue;

        int u = get_pin_id(ipin);
        //int i_loc = ipin_local[ipin];
        auto itI = canon.i_idx.find(i);
        if (itI == canon.i_idx.end()) {
          // This means your instance pin list and lib pin list disagree (naming issue)
          throw std::runtime_error("COMB arc input pin not in canon: cell=" + cellname + " pin=" + i);
        }
        int i_loc = itI->second;

        for (const auto& opin : opins) {
          std::string o = local_pin(opin);
          auto it_o = it_i->second.find(o);
          if (it_o == it_i->second.end()) continue;
          if (!it_o->second.hasData) continue;

          int v = get_pin_id(opin);
          //int o_loc = opin_local[opin];
          auto itO = canon.o_idx.find(o);
          if (itO == canon.o_idx.end()) {
            throw std::runtime_error("COMB arc output pin not in canon: cell=" + cellname + " pin=" + o);
          }
          int o_loc = itO->second;

          uint64_t key = pack_edge(u, v);
          if (arc_seen.insert(key).second) {
            G.arc_src.emplace_back(u);
            G.arc_dst.emplace_back(v);

            G.arc_kind.emplace_back(uint8_t{0});
            G.arc_cell_uid.emplace_back(cell_uid);
            G.arc_i_local.emplace_back(i_loc);
            G.arc_o_local.emplace_back(o_loc);
            G.arc_clk_pin.emplace_back(-1);
          }
        }
      }
    }
    G.net_level_ptr.emplace_back((int)G.net_src.size());
    G.arc_level_ptr.emplace_back((int)G.arc_src.size());
  }

  // -------------------------
  // 4C) Explicit PO edges (driver opin -> PO port)
  //     Needed when your _wires is keyed by PO name (net),
  //     instead of having PO appear as a tap.
  // -------------------------
  for (const auto& po : _outputs) {
    auto itw = _wires.find(po);
    if (itw == _wires.end()) continue;

    int v_po = get_pin_id(po);

    for (const auto& tap : itw->second) {
      // driver is usually a gate output pin like "inst_5:ZN"
      if (!_opin2gate.count(tap)) continue;

      int u_drv = get_pin_id(tap);

      G.net_src.emplace_back(u_drv);
      G.net_dst.emplace_back(v_po);
      G.net_tap_idx.emplace_back(-1);
    }
  }

  // allocate and default
  const int N = (int)G.id2pin.size();
  G.pin_at4.assign(N * 4, 0.0f);
  G.pin_rat4.assign(N * 4, 0.0f);
  G.pin_slew4.assign(N * 4, 0.0f);

  // Per-node FF attributes
  G.node_clk_pin.assign(N, -1);
  G.node_cell_uid.assign(N, -1);
  G.node_ipin_local.assign(N, -1);
  for (const auto& [pid, ck, uid, iloc] : ff_pin_info) {
    if (pid >= 0 && pid < N) {
      G.node_clk_pin[pid] = ck;
      G.node_cell_uid[pid] = uid;
      G.node_ipin_local[pid] = iloc;
    }
  }

  // write PI AT/SLEW
  for (const auto& port : _inputs) {
    auto it = G.pin2id.find(port);
    if (it == G.pin2id.end()) { 
      std::cerr << "Warning: PI " << port << " not found in pin universe.\n";
      continue;
    }
    int pid = it->second;

    auto it_at   = _at.find(port);
    auto it_slew = _slew.find(port);
    if (it_at != _at.end()) {
      for (int j = 0; j < 4; j++) G.pin_at4[pid*4 + j] = it_at->second[j];
    }
    if (it_slew != _slew.end()) {
      for (int j = 0; j < 4; j++) G.pin_slew4[pid*4 + j] = it_slew->second[j];
    }
  }

  // write PO RAT
  for (const auto& port : _outputs) {
    auto it = G.pin2id.find(port);
    if (it == G.pin2id.end()) {
      std::cerr << "Warning: PO " << port << " not found in pin universe.\n";
      continue;
    }
    int pid = it->second;

    auto it_rat = _rat.find(port);
    if (it_rat != _rat.end()) {
      for (int j = 0; j < 4; j++) G.pin_rat4[pid*4 + j] = it_rat->second[j];
    }
  }

  // Clock period from timing constraints
  G.clock_period = _clockTP;

  return G;
}