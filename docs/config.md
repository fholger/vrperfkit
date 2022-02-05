---
permalink: /configure/
---

# Configuration guide
The mod is configured by editing the values in its config file, `vrperfkit.yml`. The most important setting is renderScale, which determines the lowered render resolution that the game will be using internally. If you have set a render resolution of e.g. 2244x2492 in SteamVR, then that's the target resolution. The internal resolution will be scaled by the value of renderScale in both dimensions. For example, if renderScale is set to 0.75, then the actual render resolution will become 1683x1869. The render is then upscaled by FSR to the original resolution of 2244x2492.

If you set a value higher than 1 for renderScale, then the game will render at the native resolution, i.e. the one configured in SteamVR. But FSR will then take this render and upscale it to a resolution multiplied by the value of renderScale in each dimension. For example, if the resolution in SteamVR is 2242x2492 and you have configured a value of 1.3 for renderScale, then the game will render at 2242x2492, but the image will be upscaled by FSR to 2915x3240.

The second relevant parameter is sharpness. Generally, the higher you set sharpness, the sharper the final image will appear. You probably want to set this value higher if you lower renderScale, but beware of over-sharpening. The default of 0.9 gives a fairly sharp result. You can increase it up to 1.0 if you like an even sharper image. But if the image is too sharp for your taste, consider experimenting with lower values.

To switch between FSR and NIS, set the parameter useNIS either to false (FSR, default) or true (NIS).