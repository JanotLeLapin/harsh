{ gcc
, gnumake
, libsndfile
, stdenv
}: stdenv.mkDerivation {
  name = "harsh";
  version = "0.1";
  src = ./.;
  nativeBuildInputs = [ gcc gnumake ];
  buildInputs = [ libsndfile ];
  buildPhase = ''
    make
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp harsh $out/bin
  '';
}
