VR Performance Toolkit
======================

Performance-oriented collection of mods for VR games.

Included mods:

* Upscaling techniques (render at lower resolution and upscale to target resolution)
  * AMD FidelityFX Super Resolution
  * NVIDIA Image Scaling
  * AMD Contrast Adaptive Sharpening
* Fixed foveated rendering (render center of image at full resolution, but drop resolution towards edges)
  * Variable Rate Shading (only for NVIDIA RTX / GTX 16xx cards)

Planned mods:

* "Fixed foveated" rendering (render fewer pixels at the edges of the screen)
  * Radial Density Masking (all GPUs, but works only with a handful of games)
* Force hidden area mask: don't render pixels at the edges that are not visible in the headset.
  Many games already use this mask, but not all. This mod will allow you to force its usage.

Supported VR runtimes:

* Oculus
* OpenVR

Supported graphics APIs:

* Direct3D 11

## Installation

Extract `dxgi.dll` and `vrperfkit.yml` next to the game's main executable.
For Unreal Engine games, this is typically `<Game>Game\Binaries\Win64\<Game>Game-Win64-Shipping.exe`.

Edit the `vrperfkit.yml` config file to your heart's content. The available options are
documented inside the config file; you'll have to experiment with them and try which options
work for your particular game.

## Build instructions

Clone the repository and init all submodules.

```
git clone https://github.com/fholger/vrperfkit.git
cd vrperfkit
git submodule init
git submodule update --recursive
```

Download the [Oculus SDK](https://developer.oculus.com/downloads/package/oculus-sdk-for-windows)
and extract `LibOVR` from the downloaded archive to the `ThirdParty` folder.

Download [NVAPI](https://developer.nvidia.com/nvapi) (requires NVIDIA developer account) and extract
the contents of the `Rxxx-developer` folder to `ThirdParty\nvapi`.

Run cmake to generate Visual Studio solution files. Build with Visual Studio. Note: Ninja does not work,
due to the included shaders that need to be compiled. This is only supported with VS solutions.
