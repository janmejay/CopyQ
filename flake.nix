{
  description = "CopyQ dev-shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      devShells."${system}" = {
        default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            kdePackages.extra-cmake-modules
            qt6.wrapQtAppsHook
            gcc
            gnumake
            pkg-config
            gdb
          ];

          buildInputs = with pkgs; [
            qt6.qtbase
            qt6.qtsvg
            qt6.qttools
            qt6.qtdeclarative
            xorg.libXfixes
            xorg.libXtst
            qt6.qtwayland
            wayland
            kdePackages.kconfig
            kdePackages.kstatusnotifieritem
            kdePackages.knotifications
          ];

          shellHook = ''
            export CC=gcc
            export CXX=g++
          '';
        };
      };
  };
}
