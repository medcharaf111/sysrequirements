# Grid Project

This is a C++ project built with Qt 6.8.1. The main purpose of this application is to compare the system's hardware to a given game's requirements. Game data is fetched through the RAWG and SteamAPI services.

## Project Structure
- `DxDiagWorker.cpp/.h`: Handles DirectX diagnostic operations.
- `GameRequirementsWorker.cpp/.h`: Handles game requirements logic.
- `main.cpp`: Main entry point.
- `CMakeLists.txt`: CMake build configuration.
- `test.cpp`: Test file.
- `icon.ico`: Application icon.
- `dxdiag_output.txt`: Output from DxDiag.

## Build Instructions
1. **Requirements:**
   - CMake
   - C++ compiler (e.g., MSVC, MinGW)
   - Qt 6.8.1
2. **Build Steps:**
   ```sh
   mkdir build
   cd build
   cmake ..
   cmake --build .
   ```

## Usage
- Run the generated executable after building.
- The application may generate or use `dxdiag_output.txt` for diagnostics.
- Game requirements are fetched from RAWG and SteamAPI for comparison.

## License
Specify your license here.

---
*Feel free to update this README with more specific details about your project!* 