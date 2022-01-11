VR Performance Toolkit
======================

Performance-oriented collection of mods for VR games.

Included mods:

* Upscaling techniques (render at lower resolution and upscale to target resolution)
  * AMD FidelityFX Super Resolution
  * NVIDIA Image Scaling
  * AMD Contrast Adaptive Sharpening

Planned mods:

* "Fixed foveated" rendering (render fewer pixels at the edges of the screen)
  * Variable Rate Shading (only for NVIDIA RTX / GTX 16xx cards)
  * Radial Density Masking (all GPUs, but works only with a handful of games)
* Force hidden area mask: don't render pixels at the edges that are not visible in the headset.
  Many games already use this mask, but not all. This mod will allow you to force its usage.

Supported VR runtimes:

* Oculus (in alpha)
* OpenVR

## Installation

Extract `dxgi.dll` and `vrperfkit.yml` next to the game's main executable.
For Unreal Engine games, this is typically `<Game>Game\Binaries\Win64\<Game>Game-Win64-Shipping.exe`.

## Build instructions

Clone the repository and init all submodules.

Download the [Oculus SDK](https://developer.oculus.com/downloads/package/oculus-sdk-for-windows)
and extract `LibOVR` from the downloaded archive to the `ThirdParty` folder.

Run cmake to generate Visual Studio solution files. Build with Visual Studio.
