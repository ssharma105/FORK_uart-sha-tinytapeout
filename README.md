![image](docs/fangs_logo.png)
# Tiny Tapeout UART + SHA Submission Repo

See see [info.md](docs/info.md) for information on the circuit and how it operates.

`Makefile` is for building FPGA and Synthesis for FPGA target (ICE40).

`make build`: Synthesis and generate bitstream.

`make prog`: program DFU available ICE40 (for the TT motherboard thing).

`Justfile` is for simulating the design.

`just s` sets up the CMake/Verilator project.

`just r` runs the simulation and generates a VCD in the `build` directory (Can be opened with tools like `gtkwave`).


