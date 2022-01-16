# About

VR games can be very demanding on your hardware. Having to render two views, one per eye, at ever-increasing
headset resolutions and high refresh rates can make even the mightiest GPU struggle to keep up.

[vrperfkit](https://github.com/fholger/vrperfkit) is a generic modification for PCVR games that you can
install for most SteamVR and Oculus titles. It provides you with a collection of tools you can use to
squeeze a bit of extra performance out of your games without sacrificing too much visual fidelity. You
will have to spend some time tweaking settings to get the most out of it for your games, but it can give
you that much needed GPU headroom to arrive at a stable frame rate.

*NOTE*: vrperfkit can only help with GPU bottlenecks. If your game is CPU limited, this will not help!
Always measure performance before and after.

# Usage

Head over to the [latest release on GitHub](https://github.com/fholger/vrperfkit/releases/latest).
Download the `vrperfkit_vX.Y.zip` file under "Assets".

Navigate to the install directory of the game you want to install vrperfkit for. Locate the game's primary
executable. From the downloaded zip file extract the `dxgi.dll` and the `vrperfkit.yml` config file and
place them next to the exe file. For more detailed instructions and tips on how to find the right place to
put the mod, see [Installation instructions](/install/).

The mod is now installed, but you need to configure it and choose what options you would like to use.
To do so, open the `vrperfkit.yml` config file in a text editor of your choice and edit away.
For more detailed instructions, see [Configuration](/configure/).
