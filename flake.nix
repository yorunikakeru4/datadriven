{
  description = "C++20 datadriven testing library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    systems.url = "github:nix-systems/default";
    treefmt-nix.url = "github:numtide/treefmt-nix";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    systems,
    treefmt-nix,
    ...
  }:
    flake-utils.lib.eachSystem (import systems) (system: let
      pkgs = import nixpkgs {inherit system;};
      lib = pkgs.lib;

      treefmtEval = treefmt-nix.lib.evalModule pkgs {
        projectRootFile = "flake.nix";
        programs.alejandra.enable = true;
        programs.clang-format.enable = true;
        programs.cmake-format.enable = true;
      };

      cppLib = pkgs.stdenv.mkDerivation {
        pname = "datadriven";
        version = "0.1.0";

        src = lib.fileset.toSource {
          root = ./.;
          fileset = lib.fileset.unions [
            ./CMakeLists.txt
            ./cmake
            ./include
            ./src
            ./tests
            ./tools
          ];
        };

        nativeBuildInputs = with pkgs; [cmake ninja pkg-config];
        buildInputs = with pkgs; [catch2_3];

        doCheck = true;
        checkPhase = ''
          runHook preCheck
          ctest --output-on-failure
          runHook postCheck
        '';

        cmakeFlags = [
          "-DCMAKE_BUILD_TYPE=Release"
          "-DFETCHCONTENT_FULLY_DISCONNECTED=ON"
          (lib.cmakeFeature "FETCHCONTENT_SOURCE_DIR_CATCH2" "${pkgs.catch2_3.src}")
        ];
      };
    in {
      packages.default = cppLib;

      apps.default = flake-utils.lib.mkApp {
        drv = cppLib;
        exePath = "/bin/datadriven-clear";
      };

      checks = {
        cpp = cppLib;
        formatting = treefmtEval.config.build.check self;
      };

      formatter = treefmtEval.config.build.wrapper;

      devShells.default = pkgs.mkShell {
        inputsFrom = [cppLib];

        packages = with pkgs;
          [
            # C++
            clang-tools
            cmake-language-server
            ccache
            catch2_3
            just
            # Formatting
            treefmtEval.config.build.wrapper
          ]
          ++ lib.optionals stdenv.isLinux [gdb];
      };
    });
}
