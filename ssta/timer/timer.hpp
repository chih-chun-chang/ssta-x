#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <omp.h>

#include <ssta/verilog/verilog.hpp>
#include <ssta/cell/cell.hpp>
#include <ssta/unit/unit.hpp>
#include <ssta/cuda/rct.cuh>
#include <ssta/cuda/utils.cuh>
#include <ssta/cuda/timing.cuh>

struct Rct {
  std::vector<std::string> names;
  std::vector<int> nodes_st;
  std::vector<int> edges_st;
  std::vector<int> edges_s;
  std::vector<int> edges_t;
  std::vector<std::string> edges_s_str;
  std::vector<std::string> edges_t_str;
  std::vector<float> ress;
  std::vector<float> caps;
  std::vector<int> ports;
  std::vector<std::string> ports_name;
  std::vector<int> taps_st;
  std::vector<int> taps;
  std::vector<std::string> taps_name;
  std::vector<float> tapCaps;
  float Mc;
  float Mr;
  std::unordered_map<std::string, int> tap2idx;  // global tap index
};

struct Timing {
  std::vector<int> cell_hasclk;
  std::vector<int> cell_type;
  std::vector<int> cell_level_ptr;
  std::vector<int> cell_blevel_ptr;
  std::vector<int> map_to_forward;
  
  // for FF
  std::vector<cell::TimingSense> tcell_timing_sense;
  std::vector<float> tcell_timing_data_slew;  // 18
  std::vector<float> tcell_timing_data_delay; //  18
  std::vector<cell::EdgeType> tcell_constraint_setup_type;
  std::vector<float> tcell_constraint_setup_data; // 6
  std::vector<cell::EdgeType> tcell_constraint_hold_type;
  std::vector<float> tcell_constraint_hold_data; // 6
  std::vector<int> tcell_timing_ptr;
  std::vector<int> tcell_constraint_ptr;
  // for combinational cell
  std::vector<cell::TimingSense> cell_arch_sense;
  std::vector<float> cell_arch_data_slew; // 18
  std::vector<float> cell_arch_data_delay; // 18
  std::vector<int> cell_arch_ptr;
  
  std::vector<std::string> ipin_string;
  std::vector<std::string> opin_string;
  std::vector<std::string> nipin_string;
  std::vector<int> ipin; 
  std::vector<int> ipin_ptr; // PI=1 inst=numipin (should = # cell arch)
  std::vector<int> ipin_level_ptr;
  std::vector<float> ipin_timing_data_slew; // 32
  std::vector<float> ipin_timing_data_at; // 32
  std::vector<float> ipin_timing_data_rat; // 32
  std::vector<int> wp;
  std::vector<int> nipin;
  std::vector<int> nipin_ptr;
  std::vector<int> bnipin_ptr;
  std::vector<int> nipin_level_ptr;

  std::vector<int> opin; 
  std::vector<int> opin_ptr;
  std::vector<int> opin_level_ptr;
  std::vector<int> bnipin_ptr_each_gate;
  std::vector<float> opin_timing_data_slew; // 32
  std::vector<float> opin_timing_data_at; // 32
  std::vector<float> opin_timing_data_rat; // 32

  std::vector<std::string> output_string;
  std::vector<float> output_timing_data_slew;
  std::vector<float> output_timing_data_at;
  std::vector<float> output_timing_data_rat;

  std::vector<int> ipint_ptr;
  std::vector<float> ipint_timing_data_slew; // 32
  std::vector<float> ipint_timing_data_at; // 32
  std::vector<float> ipint_timing_data_rat; // 32

  std::vector<int> opin_index; // to correct lookup the opin
  std::vector<int> outputs_ipin;
};

// Debug
struct DE_Rct {
   
  std::vector<int> distances;
  std::vector<int> counts;
  std::vector<int> orders;
  std::vector<int> outputs;
  std::vector<int> pid;
  std::vector<float> pres;
  std::vector<float> load;
  std::vector<float> delay;
  std::vector<float> slew;
  std::vector<float> tapDelay;
  std::vector<float> tapSlew;
  std::vector<float> outCap;

};

////////////////////////////
// Build canonical per-cell pin orders once (per cell type) sort by pin name
struct CanonPins {
  std::vector<std::string> ipins;  // lib pin names (e.g. "A1")
  std::vector<std::string> opins;  // lib pin names (e.g. "ZN")
  std::unordered_map<std::string,int> i_idx;
  std::unordered_map<std::string,int> o_idx;
};
// export struct for python
struct PinGraph {
  // Pin ID mapping
  std::vector<std::string> id2pin;
  std::unordered_map<std::string, int> pin2id;

  // Gate-level schedule (flattened)
  std::vector<int> level_ptr;        // size L+1
  std::vector<int> level_gate_ids;   // size = total gates in forward order
  std::vector<std::string> id2gate;  // gate_id -> gate name
  std::unordered_map<std::string,int> gate2id;

  // Per-level net edges (opin -> sink ipin)
  std::vector<int> net_level_ptr;  // size L+1
  std::vector<int> net_src;        // flattened src pin_id
  std::vector<int> net_dst;        // flattened dst pin_id
  std::vector<int> net_tap_idx;
  std::vector<std::string> tap_id2pin; // size = num_taps
  std::vector<std::string> port_id2pin; // size = num_ports
  std::vector<int> net_id;

  // Per-level arc edges (ipin -> opin) within gates
  std::vector<int> arc_level_ptr;  // size L+1
  std::vector<int> arc_src;
  std::vector<int> arc_dst;

  // Optional convenience
  std::vector<int> pi_ids;
  std::vector<int> po_ids;

  // per-pin timing constraints aligned with id2pin
  std::vector<float> pin_at4;    // size = num_pins * 4
  std::vector<float> pin_rat4;   // size = num_pins * 4
  std::vector<float> pin_slew4;  // size = num_pins * 4

  // arc metadata array
  std::vector<uint8_t> arc_kind;    // size = arc_src.size(), 0:combinational, 1:sequential
  std::vector<int>     arc_cell_uid;
  std::vector<int>     arc_i_local;
  std::vector<int>     arc_o_local;

  // cell
  std::vector<int> cell_numi;
  std::vector<int> cell_numo;
  std::vector<uint8_t> cell_is_seq; // 0 Pos, 1 Neg, 2 Non/Unknown

  // comb tables
  std::vector<int> cell_arch_ptr;          // size = num_cells
  std::vector<uint8_t> cell_arch_sense;    // size = total_pairs
  std::vector<float> cell_arch_delay;      // size = total_pairs * 18
  std::vector<float> cell_arch_slew;       // size = total_pairs * 18

  // seq tables (CK->Q)
  std::vector<int> tcell_ptr;              // size = num_cells
  std::vector<uint8_t> tcell_sense;        // size = total_seq_opins
  std::vector<float> tcell_delay;          // size = total_seq_opins * 18
  std::vector<float> tcell_slew;           // size = total_seq_opins * 18
  
  // Setup/hold constraints for sequential cells [per (cell_uid, ipin, rise/fall)]
  // Layout: for each cell_uid, numi entries, each entry has rise[3] + fall[3]
  // setup_ptr[uid] = base index into setup_data
  std::vector<int>   setup_ptr;     // [num_uids] → index into setup_data
  std::vector<float> setup_data;    // [total_ipins * 6] — rise[g,h,j] + fall[g,h,j]
  std::vector<int>   hold_ptr;      // same structure
  std::vector<float> hold_data;

  // Per-arc clock pin mapping (for FF data pin backward)
  std::vector<int>   arc_clk_pin;   // [E_arc] → pin_id of clock pin (-1 if not FF)
  std::vector<int> node_clk_pin;     // [N] → -1 or clock pin_id
  std::vector<int> node_cell_uid;    // [N] → -1 or cell uid
  std::vector<int> node_ipin_local;  // [N] → -1 or local ipin index

  // Clock period
  float clock_period = 0.0f;

};

struct TimingResult {
  double forward_ms;
  double backward_ms;
};
////////////////////////////



class Timer {
  public:
    Timer() = default;

    void run() {
      std::cout << "Processing started..." << std::endl;
      std::cout << "Processing completed!" << std::endl;

      auto start = std::chrono::high_resolution_clock::now();
      _build_connections(20); // forward and backward levels
      auto stop = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
      std::cout << "Levelization Time: " << duration.count() << " ms" << std::endl;
      
      _get_arch_timing();
      _run_cuda();
    }

    Timer& read_celllib(std::filesystem::path path);
    Timer& read_verilog(std::filesystem::path path);
    Timer& read_spef(std::filesystem::path path);
    Timer& read_timing(std::filesystem::path path);

    // export function for python
    PinGraph export_levelized_pin_deps() const;
    RctCUDA computeWireTiming();
    static CanonPins build_canon_pins(const cell::Cell* cellp);

    const Rct& get_rct() const { return _rct; }

    void build_connections(int num_threads) { _build_connections(num_threads); }
    TimingResult run_timed();

  private:
  
    // circuit
    std::optional<ohm_t> _resistance_unit;
    std::optional<farad_t> _capacitance_unit;
    Rct _rct;
    DE_Rct _deRct; // for debug
    std::optional<std::shared_ptr<cell::Celllib>> _celllib;
    std::unordered_map<std::string, std::string> _pin2cell; // spef
    std::unordered_map<std::string, std::string> _ipin2gate;              // ipin -> gate (inst)
    std::unordered_map<std::string, std::string> _opin2gate;              // opin -> gate (inst)
    std::unordered_map<std::string, std::vector<std::string>> _gate2opin; // gate (inst) -> opin (wire ports)
    std::unordered_map<std::string, std::vector<std::string>> _gate2ipin; // gate (inst) -> ipin (wire taps)
    std::unordered_map<std::string, std::string> _gate2cpin; // gate (inst) -> cpin (wire taps)
    std::unordered_map<std::string, std::vector<std::string>> _wires;     // port -> taps
    std::unordered_map<std::string, std::string> _rwires;                 // taps -> port
    std::unordered_map<std::string, std::string> _gate2cell;              // gate -> cell
    std::unordered_map<std::string, bool> _hasclk;
    std::unordered_map<std::string, int> _numipins;
    std::unordered_map<std::string, int> _numopins;
    std::unordered_map<std::string, bool> _isvisited;
    std::unordered_map<std::string, std::string> _clknet;
    std::vector<std::string> _inputs;
    std::vector<std::string> _outputs;
    std::vector<std::string> _gates;
    std::unordered_map<std::string, int> _num_visited;
    std::unordered_map<std::string, int> _num_fanout;
    std::queue<std::string> _q, _q2, _qback;
    std::vector<std::string> _vback;

    // timer
    Timing _t;
    std::map<int, std::vector<std::string>> _forward;
    std::map<int, std::vector<std::string>> _backward;

    // timing
    std::string _clockNode;
    float _clockTP;
    std::unordered_map<std::string, std::array<float, 4>> _at;
    std::unordered_map<std::string, std::array<float, 4>> _rat;
    std::unordered_map<std::string, std::array<float, 4>> _slew;

    // func
    void _verilog(vlog::Module& module);
    void _read_spef(spef::Spef& spef);
    void _rebase_unit(spef::Spef& spef);
    void _build_connections(int num_threads);
    void _get_arch_timing();
    void _debug();
    

    // cuda
    RctCUDA _copy_rct2gpu();
    TimingCUDA _copy_timing2gpu();
    void _copy_rct2cpu(RctCUDA& rct_cuda);
    void _run_cuda();
    void _close_cuda(RctCUDA& rct_cuda, TimingCUDA& timing_cuda);

    
};


