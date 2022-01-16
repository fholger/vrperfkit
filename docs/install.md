---
permalink: /install/
---

# Installation instructions

These instructions will guide you through installing vrperfkit for a game of your choice.

### Step 1 - Download vrperfkit

Head over to the [latest release on GitHub](https://github.com/fholger/vrperfkit/releases/latest).
Download the `vrperfkit_vX.Y.zip` file under "Assets". Extract the contents of the zip file to a
folder of your choice.

### Step 2 - Locate your game's install path

Open Windows Explorer and navigate to the installation directory of the game you want to install
vrperfkit for.

For Steam games, this is usually `C:\Program Files (x86)\Steam\steamapps\common\<Game>`. You can also
right-click on the game in Steam and choose "Manage" -> "Browse local files"

![Steam Browse local files](/images/steam_find_install_location.jpg)

For Oculus games, look under `C:\Program Files\Oculus\Software\<Game>`. Oculus titles often have also
very convoluted name for their directory, often pieced together by the developer's name and a project
code name, which can make identifying the correct folder for a game difficult. In the Oculus software,
you can click on the three `...` next to a title and select "Details". In the overlay, it will show
you the install path.

### Step 3 - Find the game's primary executable

Now you need to find the primary \*.exe file in the game's installation folder. Often, it will be right
at the root and called something like `<Game>.exe`, but not always. Some games use launchers, which are
smaller executables that launch another .exe somewhere deeper in the folder hierarchy. If you spot an
.exe that's very small (e.g. smaller than 1 Mb), it's probably just a launcher, and you need to keep
digging. Look for folders called `bin` or similar and go through their subfolders until you find
a promising executable.

One particular example of games with launchers are Unreal Engine 4 games. You can easily identify
them because in their installation directory you will find two subfolders, one called `Engine` and one
called `<Game>Game`. Navigate to `<Game>Game\Binaries\Win64` where you should find an executable called
`<Game>Game-Win64-Shipping.exe`. You are in the right place.

### Step 4 - Copy over the necessary vrperfkit files

From the location where you extracted the contents of the vrperfkit zip file, copy the two files
`dxgi.dll` and `vrperfkit.yml` and place them next to the game's main executable.

### Step 5 - Test the installation

Now launch the game and quit it right after it has launched. Look for a log file `vrperfkit.log` that
should have been created right next to primary executable (where you just copied the mod files). If the
log file is there, congratulations - you've successfully installed the mod!

If it's not there, you may not have put the mod files in the right place. Go back to step 3 and look for
another candidate executable. If you still can't get it to work, it's also possible that the game is
using an unsupported rendering API and won't load vrperfkit no matter what. In that case, unfortunately
your game is currently incompatible with vrperfkit.

### Step 6 - Configure the mod to your heart's content

Now that you've successfully installed vrperfkit for your game, you need to configure it. Head over to
the [Configuration guide](/configure/) and follow the instructions there.
