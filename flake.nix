{
  description = "a toy DAW written in C";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs";

  outputs = {
    nixpkgs,
    ...
  }:
  let
    eachSystem = fn: nixpkgs.lib.genAttrs [
      "x86_64-linux"
      "aarch64-linux"
    ] (system: let
      pkgs = (import nixpkgs { inherit system ; });
    in (fn { inherit system pkgs; }));
  in {
    devShells = eachSystem ({ pkgs, ... }: { default = pkgs.callPackage ./shell.nix {}; });
    packages = eachSystem ({ pkgs, ... }: {
      default = pkgs.callPackage ./default.nix {};
    });
  };
}
