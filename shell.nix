{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    gcc
    dpp
    nlohmann_json
    imagemagick
    unzip
  ];

  nativeBuildInputs = with pkgs; [
    clang-tools
    codespell
    lcov
  ];

  shellHook = ''
    echo "srb2dbot build environment"
    echo "  cmake  : $(cmake --version | head -1)"
    echo "  g++    : $(g++ --version | head -1)"
    echo ""
    echo "To build:"
    echo "  cmake -B build -DCMAKE_BUILD_TYPE=Debug"
    echo "  cmake --build build"
    echo "To test:"
    echo "  cd build && ctest"
  '';
}
