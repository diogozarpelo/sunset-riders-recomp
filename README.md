# Sunset Riders Recomp

<p align="center">
  An experimental PC port of Sunset Riders for the Super Nintendo, built with SNESRecomp and a shared desktop launcher.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C-C11-A8B9CC?logo=c&logoColor=black" alt="C11">
  <img src="https://img.shields.io/badge/SDL-3-1466B8" alt="SDL3">
  <img src="https://img.shields.io/badge/CMake-Build-064F8C?logo=cmake&logoColor=white" alt="CMake">
  <img src="https://img.shields.io/badge/Windows-x64-0078D4" alt="Windows x64">
  <img src="https://img.shields.io/badge/Status-Alpha-EA580C" alt="Alpha">
</p>

## About the project

**Sunset Riders Recomp** brings the SNES version of Sunset Riders to a desktop application with a configurable launcher, video and audio output, and controller support.

The project uses **SNESRecomp** for ROM analysis, local code generation, and the SNES runtime. A game-specific host coordinates guest execution, frame timing, interrupts, and rendering. The launcher is provided by **recomp-ui**.

The current implementation relies on the framework's low-level interpreter bridge for game execution. Recompilation infrastructure is integrated, but this alpha is not a complete conversion of all game routines to native C.

This is an unofficial project. The original game's ROM and assets are not included. Each user must supply their own compatible ROM obtained lawfully.

## Main features

- Desktop launcher with ROM selection and verification.
- Display, audio, and controller configuration through the shared launcher.
- Optional launcher bypass on subsequent starts.
- Game-specific execution and frame scheduling.
- Working video and audio in the tested Windows build.
- Controller support, manually verified with a wireless Xbox Series X controller.
- ROM identity checks using SHA-256 and CRC32.
- Local generation of ROM-derived files, excluded from version control and release packages.
- Pinned framework dependencies and a reproducible MSVC compatibility patch.

## Status

**Early alpha, intended for testing and continued development.** The first public release is planned as **v0.1.0-alpha**.

The game has been **tested through the full campaign** on Windows x64. Normal life loss and Game Over behavior were also verified.

Known visual problems remain, and one reported issue may affect gameplay. Full campaign completion does not imply exhaustive validation of every character, multiplayer mode, configuration, or platform.

The ROM-free setup package still requires validation from a clean installation before it can be recommended as the primary installation method.

## Supported ROM

The tested and supported image is **Sunset Riders (USA)** for the SNES. Other regions and revisions have not been validated.

| Property | Expected value |
|---|---|
| Game | Sunset Riders |
| Platform | Super Nintendo Entertainment System |
| Region | USA / North America |
| Filename hint | `Sunset Riders (USA).sfc` |
| Size without copier header | 1,048,576 bytes |
| Mapping | LoROM |
| CRC32 | `52ada404` |
| SHA-256 | `e9c406d4f773697b9b671e7ddf2207c9d0ab242d7f23e502cdd453fbb264d392` |

The hashes identify the supported image; a matching filename alone does not establish compatibility. These values refer to the ROM without a copier header. Use a headerless image when following the instructions below.

The authoritative identity is stored in [`rom_identity.txt`](rom_identity.txt) and shared by the build, generation script, and packaging script.

**No ROM is provided, and this repository does not link to ROM downloads.**

## Installation and local build

### Requirements

The following environment was used for the tested Windows build:

- Windows x64.
- Git for Windows, including Git Bash.
- Python 3 available as `python` in PowerShell.
- Rust and Cargo for the framework's recompiler tooling.
- CMake 3.20 or newer, available as `cmake` in PowerShell.
- Visual Studio 2022 Build Tools with **Desktop development with C++**, the MSVC toolset, and a Windows SDK.
- Internet access for initial dependency downloads.
- Your own compatible Sunset Riders (USA) ROM.

SDL3 is fetched and built by CMake if a suitable installed package is not available. Other operating systems have not been validated for this alpha.

### Clone the project

Use PowerShell:

```powershell
git clone --recurse-submodules https://github.com/diogozarpelo/sunset-riders-recomp.git
if ($LASTEXITCODE -ne 0) { throw 'Clone failed.' }
Set-Location sunset-riders-recomp
```

For an existing checkout, initialize the dependencies:

```powershell
git submodule update --init --recursive
if ($LASTEXITCODE -ne 0) { throw 'Submodule initialization failed.' }
```

Use the committed dependency revisions. Updating to arbitrary upstream versions may change runtime behavior or break compatibility with the local patch.

### Generate local game files

Keep your ROM outside the repository. Replace the example path below with your own file:

```powershell
$rom = (Resolve-Path 'C:\ROMs\Sunset Riders (USA).sfc').Path.Replace('\', '/')
$env:PYTHON = (Get-Command python -ErrorAction Stop).Source.Replace('\', '/')
& 'C:\Program Files\Git\bin\bash.exe' ./tools/regen.sh --rom $rom
if ($LASTEXITCODE -ne 0) { throw 'ROM verification or code generation failed.' }
```

The script verifies the ROM and produces `src/gen/` and `recomp/funcs.h` locally. These files are derived from the ROM and must not be committed or distributed with this project.

### Configure and compile

```powershell
cmake -S . -B build -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
cmake --build build --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
```

CMake applies the project's PPU alignment patch when compiling with MSVC. If it is already installed, configuration leaves it in place. An incompatible framework checkout produces an explicit error.

The executable is generated at:

```text
build/Release/SunsetRidersSNESRecomp.exe
```

### Open the launcher

From the repository root, use the ROM path defined above:

```powershell
& .\build\Release\SunsetRidersSNESRecomp.exe --launcher $rom
```

Choose the ROM if requested, configure your display, audio, and controller preferences, then select **Play**. Connect your controller before launching for the most predictable device detection.

To start directly with the same ROM:

```powershell
& .\build\Release\SunsetRidersSNESRecomp.exe $rom
```

Keep the staged runtime files and directories alongside the executable. Moving only the `.exe` is not the supported distribution method.

## Release packaging

The repository includes [`scripts/package_release.sh`](scripts/package_release.sh) for producing a **ROM-free setup pack**.

The intended package contains a setup host, project sources, and recompiler tooling. It excludes the ROM, generated game code, and `recomp/funcs.h`. The launcher is designed to generate and rebuild locally from the player's selected ROM.

A setup host is distinct from the playable executable produced by the local build above. The packaging script requires a build configured with `-DSNESRECOMP_SETUP_HOST=ON` and no generated game translation units.

End-to-end setup-pack installation is still pending validation. Until that flow has been tested, use the source-build instructions above. Download instructions for a published package will be added when that package is available and verified.

## Known issues

The following issues were reported during campaign testing:

| Area | Observed issue | Impact |
|---|---|---|
| Introduction | Minor graphical glitches affecting characters during the intro. | Visual |
| Dark Horse stage | Fire effects display incorrectly. | Visual; further investigation needed |
| Saloon sequence | The dancers' animation displays incorrectly after defeating the brothers. | Visual |
| Train stage | Poles appear misplaced near the middle of the screen and seem to lack the expected collision behavior. | Potential gameplay impact; collision behavior needs confirmation |
| Final stage | Fire effects show a problem similar to the Dark Horse stage. | Visual; further investigation needed |

The causes have not yet been isolated. Similar symptoms do not establish that the fire effects share the same underlying defect.

For a useful bug report, include the stage, character, reproduction steps, build version, operating system, and relevant runtime logs. Screenshots or a short recording are helpful. Do not attach ROM files or generated game code.

## Technologies

| Component | Technology / responsibility |
|---|---|
| Game host | C11, execution scheduling and frame integration |
| SNES runtime and tooling | SNESRecomp |
| Desktop video, audio, and input | SDL3 |
| Launcher | recomp-ui / Dear ImGui |
| Build | CMake, Visual Studio 2022 / MSVC |
| ROM verification and generation | Python tooling and the framework's recompiler |
| Development scripts | Bash and PowerShell |
| Version control | Git, GitHub, pinned submodules |

## Project structure

| Path | Purpose |
|---|---|
| `src/` | Game host, desktop entry point, and framework integration |
| `src/gen/` | Locally generated code; ignored by Git |
| `recomp/` | Analysis configuration and symbol definitions |
| `cmake/` | Project build helpers |
| `patches/` | Compatibility fixes for pinned dependencies |
| `tools/regen.sh` | ROM verification and local generation |
| `scripts/package_release.sh` | ROM-free setup-pack staging |
| `snesrecomp/` | Pinned framework submodule |
| `recomp-ui/` | Pinned launcher submodule |
| `rom_identity.txt` | Supported ROM identity |
| `framework_pins.txt` | Framework revision reference |
| `VERSION` | Build version |

## Testing and quality

Manual validation on Windows x64 includes:

- Boot and introduction playback.
- Video and audio output.
- Launcher settings and controller configuration.
- Character selection and gameplay.
- Full campaign completion.
- Normal life loss and Game Over behavior.
- Wireless Xbox Series X controller detection and use.

The MSVC build also succeeds with the compatibility patch integrated into CMake. Fresh-install packaging, broader controller coverage, multiplayer, and exhaustive character testing remain to be validated.

## Roadmap

- Investigate train-stage pole placement and collision behavior.
- Correct fire rendering in the Dark Horse and final stages.
- Correct intro and saloon animation glitches.
- Validate the setup pack from a clean Windows environment.
- Expand character, controller, and multiplayer testing.
- Add real launcher and gameplay screenshots.
- Improve runtime integration and progressively expand recompiled coverage.

## Author

**Diogo Antonio Zarpelão**

Sunset Riders port integration, game-host changes, Windows build configuration, gameplay testing, and project documentation.

GitHub: [@diogozarpelo](https://github.com/diogozarpelo)

## Credits and license

- [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp): framework, runtime, and recompiler tooling.
- [recomp-ui](https://github.com/mstan/recomp-ui): shared launcher, developed by Matthew Stanley and contributors.
- SDL and Dear ImGui: desktop and user-interface dependencies, under their respective licenses.

The project retains the **PolyForm Noncommercial License 1.0.0** supplied with the scaffold. See [`LICENSE`](LICENSE) for the full terms and original notices. The pinned launcher is distributed under the MIT License; dependency license and copyright notices must be preserved.

The project's license does not grant rights to the original game's ROM, artwork, music, or other copyrighted game content. No such content is included in this repository or its intended setup packages.
