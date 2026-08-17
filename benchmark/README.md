# Benchmarks

## What is bundled

`tau2013_contest_library.v1.0.lb` is the TAU 2013 contest cell library, used by
every design here.

`tau2015/` holds 15 small ISCAS-85 and ISCAS-89 designs, each with the three
files SSTA-X reads:

| File | Contents |
| --- | --- |
| `<design>.v` | gate-level Verilog netlist |
| `<design>.spef` | interconnect parasitics |
| `<design>.timing` | arrival time, required arrival time, slew, and clock assertions |

Designs: `c17`, `c1355`, `c1908`, `c2670`, `c5315`, `c6288`, `c7552`, `s27`,
`s344`, `s349`, `s400`, `s510`, `s526`, `s1196`, `s1494`.

Run any of them with:

```bash
./build/ssta-x -l benchmark/tau2013_contest_library.v1.0.lb -d benchmark/tau2015/c6288
```

## The large designs

The industrial designs the paper reports speedups on are not bundled — together
they run to roughly 11 GB, dominated by their SPEF files:

`ac97_ctrl`, `aes_core`, `b19_iccad`, `cordic_ispd`, `des_perf`, `leon2_iccad`,
`leon3mp_iccad`, `netcard_iccad`, `vga_lcd_iccad`, `mgc_edit_dist_iccad`,
`mgc_matrix_mult_iccad`.

They are distributed by the TAU contests:

- TAU 2015 contest (incremental timing analysis): https://sites.google.com/site/taucontest2015/
- TAU 2014 contest: https://sites.google.com/site/taucontest2014/
- TAU 2013 contest: https://sites.google.com/site/taucontest2013/

To use one, drop its directory under `benchmark/tau2015/` keeping the
`<design>/<design>.{v,spef,timing}` naming, and it will work with `--design`
like the bundled ones. These paths are already listed in `.gitignore` so large
downloads are not accidentally committed.
