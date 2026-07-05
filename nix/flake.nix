{
  description = "Flake for building a Raspberry Pi Zero 2 SD image";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-26.05";
    flake-utils.url = "github:numtide/flake-utils";
    deploy-rs.url = "github:serokell/deploy-rs";
  };

  outputs = inputs: with inputs; flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = nixpkgs.legacyPackages."${system}";
      crossPkgs = import "${nixpkgs}" { 
        localSystem = system;
        crossSystem = "aarch64-linux";
      };
      soundboard_cpp = pkgs.callPackage ./soundboard-cpp.nix {};
    in rec {
      packages = {
        default = soundboard_cpp;
        soundboard_cpp = soundboard_cpp;
        soundboard_python = pkgs.callPackage ./soundboard-python.nix {};
      };

      nixosConfigurations = {
        zero2w = nixpkgs.lib.nixosSystem {
          modules = [
            "${nixpkgs}/nixos/modules/installer/sd-card/sd-image-aarch64.nix"
            ./zero2w.nix { nixpkgs.pkgs = crossPkgs; }
          ];
        };
        testvm = nixpkgs.lib.nixosSystem {
          system = system;
          modules = [ ./testvm.nix ];
        };
      };

      deploy = {
        user = "root";
        nodes = {
          zero2w = {
            hostname = "zero2w";
            profiles.system.path =
              deploy-rs.lib.aarch64-linux.activate.nixos self.nixosConfigurations.zero2w;
          };
        };
      };
    }
  );
}
