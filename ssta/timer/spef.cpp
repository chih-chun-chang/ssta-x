#include <ssta/timer/timer.hpp>

Timer& Timer::read_spef(std::filesystem::path path) {
  auto spef = std::make_shared<spef::Spef>(); 

  if(spef->read(path); spef->error) 
    std::cerr << "Parser-SPEF error: " << *spef->error << std::endl;  
    
  spef->expand_name();

  if(!(spef->error)) {
    _rebase_unit(*spef);
    _read_spef(*spef);
  }

  return *this;
}

void Timer::_read_spef(spef::Spef& spef) {
  std::size_t num_nets = spef.nets.size();
  _rct.names.reserve(num_nets);
  _rct.nodes_st.reserve(num_nets+1);
  _rct.edges_st.reserve(num_nets+1);
  _rct.ports.reserve(num_nets);
  _rct.taps_st.reserve(num_nets+1);

  if (!_celllib) 
    throw std::runtime_error("celllib not loaded!\n");
  auto cl = _celllib.value();
  
  float Mr = cl->metal3ResScaleFactor;
  float Mc = cl->metal3CapScaleFactor;

  int max_num_nodes = 0;
  std::unordered_map<std::string, std::size_t> stoi;
  std::size_t nodes_st {0};
  std::size_t edges_st {0};
  std::size_t taps_st {0};
  for (auto& spef_net : spef.nets) {
    _rct.names.emplace_back(spef_net.name);
    
    // hash table for mapping string nodes to unique indices
    stoi.clear();
    std::size_t index = 0;

    // start index of each net
    _rct.nodes_st.emplace_back(nodes_st);
    _rct.edges_st.emplace_back(edges_st);
    _rct.taps_st.emplace_back(taps_st);
    
    // add nodes to hash table
    for (int i = 0; i < spef_net.connections.size(); ++i)
      if (stoi.find(spef_net.connections[i].name) == stoi.end()) 
        stoi[spef_net.connections[i].name] = index++;

    for (const auto& [n1, n2, v] : spef_net.caps)
      if (stoi.find(n1) == stoi.end()) stoi[n1] = index++;
    
    for (const auto& [n1, n2, v] : spef_net.ress) {
      if (stoi.find(n1) == stoi.end()) stoi[n1] = index++;
      if (stoi.find(n2) == stoi.end()) stoi[n2] = index++;
    }
    
    // node mapping
    for (int i = 0; i < stoi.size(); ++i)
      _rct.caps.emplace_back(0);
    for (const auto& [n1, n2, v] : spef_net.caps) // n2 should be null
      _rct.caps[nodes_st + stoi[n1]] = v*static_cast<double>(_capacitance_unit.value());

    for (const auto& [n1, n2, v] : spef_net.ress) {
      if (stoi.find(n1) == stoi.end() || stoi.find(n2) == stoi.end())
        std::cerr << n1 << " or " << n2 << " not found in the hashmap\n";
      else {
        _rct.edges_s.emplace_back(stoi[n1]);
        _rct.edges_t.emplace_back(stoi[n2]);
        _rct.edges_s_str.emplace_back(n1);
        _rct.edges_t_str.emplace_back(n2);
        _rct.ress.emplace_back(v*static_cast<double>(_resistance_unit.value()));
      }
    }
    
    // assume that the first one shall be the port and the remaining are taps
    std::string portName, tapName;
    for (std::size_t i = 0; i < spef_net.connections.size(); ++i) {
      if (i == 0) { // port
        portName = spef_net.connections[i].name;
        _rct.ports.emplace_back(stoi[spef_net.connections[i].name]);
        _rct.ports_name.emplace_back(spef_net.connections[i].name);
      }
      else { // taps
        tapName = spef_net.connections[i].name;
        _wires[portName].emplace_back(tapName);
        _rwires[tapName] = portName;
        _rct.taps.emplace_back(stoi[spef_net.connections[i].name]);
        _rct.taps_name.emplace_back(spef_net.connections[i].name);
        _rct.tap2idx[spef_net.connections[i].name] = (int)_rct.taps.size() - 1;
        // lookup cell library
        std::string tap_name = spef_net.connections[i].name;
        if (_pin2cell.find(tap_name) != _pin2cell.end()) {
          std::string cell_name = _pin2cell[tap_name];
          size_t pos = tap_name.find(':');
          if (pos != std::string::npos) {
            std::string pin_name = tap_name.substr(pos + 1);
            _rct.tapCaps.emplace_back(cl->lib[cell_name]->iPinFallCap[pin_name]);
            _rct.tapCaps.emplace_back(cl->lib[cell_name]->iPinRiseCap[pin_name]);
          }  
          else {
            throw std::runtime_error("pin not found\n");
          }
        }
        else {
          _rct.tapCaps.emplace_back(0);
          _rct.tapCaps.emplace_back(0);
        }
      }
    }
    //std::cout << std::endl;
    max_num_nodes = stoi.size() > max_num_nodes ? stoi.size() : max_num_nodes;
    nodes_st += stoi.size();
    edges_st += spef_net.ress.size();
    taps_st += spef_net.connections.size() - 1;
  } // end for spef_net
  _rct.nodes_st.emplace_back(nodes_st);
  _rct.edges_st.emplace_back(edges_st);
  _rct.taps_st.emplace_back(taps_st);
}

// Procedure: _rebase_unit
void Timer::_rebase_unit(spef::Spef& spef) 
{
  auto resu = make_resistance_unit(to_lower(spef.resistance_unit));
  auto capu = make_capacitance_unit(to_lower(spef.capacitance_unit));

  // Convert the capacitive load unit
  if(!_capacitance_unit) 
    _capacitance_unit = capu;
  else if(capu) 
  {
    float s = (*capu / *_capacitance_unit).value();
    if(std::fabs(s - 1.0f) >= 1e-2f) 
    {
      std::cout << "rebase spef capacitance to " << *capu << std::endl; 
      spef.scale_capacitance(s);
    }
  }

  // conver teh resistance unit
  if(!_resistance_unit) 
    _resistance_unit = resu;
  else if(resu)
  {
    float s = (*resu / *_resistance_unit).value();
    if(std::fabs(s - 1.0f) >= 1e-2f) 
    {
      std::cout << "rebase spef resistance to " << *resu << std::endl; 
      spef.scale_resistance(s);
    }
  }
  
}
