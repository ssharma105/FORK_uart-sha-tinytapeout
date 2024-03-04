{
  outputs = {nixpkgs, ...}: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};

    stdenv = pkgs.llvmPackages_17.stdenv;

    pkgs-stdenv = import nixpkgs {
      inherit system;
      overlays = [
        (new: prev: {verilator = prev.verilator.override {inherit stdenv;};})
      ];
    };

    mkShell = pkgs.mkShell.override {inherit stdenv;};
  in {
    devShells.${system}.default = mkShell {
      CPLUS_INCLUDE_PATH = "${pkgs-stdenv.verilator}/share/verilator/include";
      packages = with pkgs; [
        # build
        gnumake
        cmake
        ninja
        just

        # pnr
        yosys
        icestorm
        nextpnr

        # tb / sim
        pkgs-stdenv.verilator
        gtkwave

        # program
        dfu-util

        # dev
        svls
        svlint
        verible
        clang-tools
      ];
    };
  };
}
