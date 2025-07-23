{ nil
, clang-tools
, valgrind
, mkShell
}: mkShell {
  buildInputs = [
    nil clang-tools
    valgrind
  ];
}
