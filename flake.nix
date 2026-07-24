{
  description = "drmenu - a radial/pie menu launcher for Wayland compositors";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages = rec {
          drmenu = pkgs.callPackage ./nix/package.nix { };
          default = drmenu;
        };
      }
    )
    // {
      overlays.default = final: prev: {
        drmenu = final.callPackage ./nix/package.nix { };
      };
    };
}
