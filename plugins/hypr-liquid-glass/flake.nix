{
  inputs = {
    hyprland = {
      type = "github";
      owner = "hyprwm";
      repo = "Hyprland";
      ref = "v0.56.1";
    };

    nixpkgs.follows = "hyprland/nixpkgs";
  };

  outputs =
    {
      self,
      nixpkgs,
      hyprland,
      ...
    }:
    let
      withPkgsFor =
        fn:
        nixpkgs.lib.genAttrs (builtins.attrNames hyprland.packages) (
          system:
          fn system (
            import nixpkgs {
              inherit system;
              overlays = [
                hyprland.overlays.hyprland-packages
                self.overlays.default
              ];
            }
          )
        );
    in
    {
      packages = withPkgsFor (
        system: pkgs: rec {
          inherit (pkgs.hyprlandPlugins) hypr-liquid-glass;

          default = hypr-liquid-glass;
        }
      );

      overlays = {
        default = self.overlays.hypr-liquid-glass;

        hypr-liquid-glass = final: prev: {
          hyprlandPlugins = (prev.hyprlandPlugins or { }) // {
            hypr-liquid-glass = final.callPackage ./default.nix { };
          };
        };
      };

      formatter = withPkgsFor (_: pkgs: pkgs.alejandra);
    };
}
