# Weather-App

A small command-line weather lookup tool written in C that uses libcurl to fetch data from IP and weather APIs and cJSON to parse responses.

Features
- Query weather for a city: `./weather-app <city name>` (supports spaces)
- Auto-locate nearest city using your public IP: `./weather-app locate`

This README explains how to install dependencies on Windows (vcpkg), Linux (apt), and macOS (brew), how to build with CMake, recommended compilers, and how to run the compiled binary.

## Dependencies

This project depends on the following libraries:

- libcurl (for HTTP requests)
- cJSON (for JSON parsing)
- CMake (build system)

Note: The source currently contains a hard-coded OpenWeatherMap API key in `src/weather.c` under the `API_KEY` macro. You should replace it with your own API key (see "API Key" below).

### Recommended compilers

- Windows: GCC from MinGW-w64 (recommended by the project owner)
- Linux: GCC (system default, e.g., `gcc` / `g++`)
- macOS (Darwin on Apple Silicon): Homebrew GCC (aarch64)

## Install dependencies

Below are step-by-step instructions for each OS. Use the section that matches your environment.

### Windows (vcpkg + MinGW)

Prereqs:

- Install MinGW-w64 (for example from MSYS2 or the MinGW-w64 installer). Make sure `gcc.exe` is available in PATH.
- Install Git and PowerShell (comes with Windows).

Install and use vcpkg (one-time):

1. Clone and bootstrap vcpkg (PowerShell):

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

2. Install libraries using vcpkg. If you use the default MSVC toolchain (Visual Studio) you can install for `x64-windows`.
	 For MinGW, attempt the MinGW triplet (availability depends on your vcpkg version):

```powershell
.\vcpkg.exe install curl cjson:x64-windows
# or if you use a MinGW triplet that exists in your vcpkg installation:
.\vcpkg.exe install curl cjson:x64-mingw
```

3. When running CMake, pass the vcpkg toolchain file so CMake can find the installed packages:

```powershell
# from the project root
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER="C:/path/to/mingw64/bin/gcc.exe" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Notes:
- If vcpkg does not provide a MinGW triplet in your version, you can either install the libraries manually using MSYS2/pacman or build/link curl and cJSON from source and point CMake to those locations.

### Linux (Debian/Ubuntu - apt)

Install build tools and libraries:

```bash
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libcjson-dev git
```

Build with CMake:

```bash
cd /path/to/Weather-App
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j$(nproc)
```

### macOS (Homebrew)

Install Homebrew if you don't have it (https://brew.sh/). Then install dependencies:

```zsh
brew update
brew install cmake curl cjson gcc
```

Note: On macOS (Apple Silicon) Homebrew's GCC is usually named like `gcc-13` (version may vary). The project owner requested GCC (aarch64) for Darwin — to use Homebrew GCC explicitly with CMake, set CC and CXX or pass -DCMAKE_C_COMPILER:

```zsh
# example if brew installed gcc-13
export CC=/opt/homebrew/bin/gcc-13
export CXX=/opt/homebrew/bin/g++-13
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j$(sysctl -n hw.ncpu)
```

If you prefer to use the system clang toolchain, you can omit setting CC/CXX.

## Build (common CMake steps)

From the repository root:

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Notes:
- If you installed libraries via vcpkg, pass `-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake` to the `cmake` invocation so CMake finds curl and cJSON.
- For MinGW on Windows, use `-G "MinGW Makefiles"` and set `-DCMAKE_C_COMPILER` to your MinGW `gcc.exe`.

## Resulting binary / release location

- After a successful build the executable will be placed in the `build/` directory. By default the compiled binary is named `weather-app` (on Windows it may be `weather-app.exe`). Example path from repository root:

	- Unix/macOS: `./build/weather-app`
	- Windows (PowerShell): `.
		\build\weather-app.exe`

- To create a system-wide install (optional), you can copy the binary to a directory in PATH, for example `/usr/local/bin/` on Unix-like systems:

```bash
sudo cp build/weather-app /usr/local/bin/
# then you can call `weather-app` from anywhere
```

## Usage

- Query a specific city (supports spaces):

```bash
./build/weather-app Tokyo
./build/weather-app "New York"
```

- Auto-locate by your public IP (finds nearest city and shows its weather):

```bash
./build/weather-app locate
```

- The program prints a readable summary including temperature (°C), humidity, pressure, wind and sunrise/sunset times.

## API Key

The OpenWeatherMap API key is currently hard-coded in `src/weather.c` as `API_KEY`. Replace it with your own key before building, or modify the code to read the API key from an environment variable if you prefer not to keep the key in source control.

To replace directly in the source, edit `src/weather.c` and change the `#define API_KEY "..."` line.

Alternatively, you can modify the code to read from an environment variable `OPENWEATHER_API_KEY` (recommended):

1. Edit `src/weather.c` to use `getenv("OPENWEATHER_API_KEY")`.
2. Export the variable before running:

```bash
export OPENWEATHER_API_KEY="your_api_key_here"
./build/weather-app Tokyo
```

## Troubleshooting

- Missing headers like `curl/curl.h` or `cjson/cJSON.h`: ensure libcurl and cJSON dev packages are installed and CMake found them. If using vcpkg, pass the toolchain file.
- Linker errors related to curl or cjson: confirm the proper library search paths are passed (vcpkg toolchain or system dev packages).
- SSL / TLS errors when calling HTTPS endpoints: on some systems you may need to install CA certificates or enable curl SSL verification. The code currently disables SSL verification in the `get_public_ip` helper; consider enabling verification in production.
- If the program prints `Failed to retrieve public IP` or `All IP services failed`, check network connectivity and that the endpoint `https://api.ipify.org` (and the alternates) are reachable.
- If weather lookup fails, verify your OpenWeatherMap API key is valid and hasn't exceeded quota.

## Security & privacy

- This app sends your public IP to third-party IP/geolocation and weather services to determine location and weather. Review those services' privacy policies before using.

## Contributing

- PRs welcome. Please avoid committing private API keys.

## Final notes

- Example run (from repo root):

```bash
./build/weather-app locate
./build/weather-app "San Francisco"
```

If you want, I can also update the code to load the API key from an environment variable and add a small CMake option to configure it at build time.

