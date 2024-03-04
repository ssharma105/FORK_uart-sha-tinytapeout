build_dir := "build/tb"
exe := "run_tb"

ensure_build:
    @mkdir -p {{ build_dir }}

alias s := setup

setup: ensure_build
    cd {{ build_dir }} && cmake ../.. -GNinja

alias b := build

build:
    ninja -C {{ build_dir }}

alias r := run

run: build
    ./build/tb/{{ exe }}
