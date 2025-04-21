{
  description = "Dev shell for DD HTTP emissions fetcher in C";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
  };

  outputs = { self, nixpkgs }: {
    devShells.x86_64-linux.default = 
      let
        pkgs = import nixpkgs {
          system = "x86_64-linux";
        };
      in
      pkgs.mkShell {
        name = "emissions-dev";

        packages = with pkgs; [
          gcc
          curl.dev
          jansson.dev
          jansson  
          clang
          clang-tools
          gnumake
        ];

        shellHook = ''
          echo "🚀 Entering C development shell with libcurl and jansson"
        '';
      };
  };
}

