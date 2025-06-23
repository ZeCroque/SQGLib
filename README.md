SQGLib is a C++ lib powered by SKSE to add runtime quest generation capabilities to Skyrim.

## Requirements
* [CMake](https://cmake.org/)
	* Add this to your `PATH`
* [The Elder Scrolls V: Skyrim Special Edition](https://store.steampowered.com/app/489830)
	* Add the environment variable `Skyrim64Path` to point to the root installation of your game directory (the one containing `SkyrimSE.exe`).
* [Vcpkg](https://github.com/microsoft/vcpkg)
	* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
* [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
	* Desktop development with C++

## Building
```
git clone https://github.com/ZeCroque/SQGLib.git
cd SQGLib
git submodule init
git submodule update
cmake --preset vs2022-windows
cmake --build build --config Release
```

## Tips
* You can add the environment variable `MO2Path` to automatically add a debug target that will launch MO2 from Visual Studio. To attach to the right process you will need to install the extension [Microsoft Child Process Debugging Power Tool 2022](https://marketplace.visualstudio.com/items?itemName=vsdbgplat.MicrosoftChildProcessDebuggingPowerTool2022)
	* Set `MO2_INSTANCE` to the name of the instance you want MO2 to manage (default is `Skyrim Special Edition`)
	* Set `MO2_TARGET` to the name of the target you want MO2 to run (default is `Skyrim Special Edition`). To debug properly the target should point to `SkyrimSE.exe` with SKSE dll loaded using the `Force load libraries` feature
	* Set `MO2_PROFILE` to the name of the MO2 profile you want to use (useful to have MO2 write to a mod folder instead of the overwrite in the chosen profile)
* Set `LEGACY_SCRIPT_PATH` to `ON` to use the old Scripts/Source path instead of the new (and stupid) Source/Scripts path
* Set `NOTES_FOLDER_PATH` if you are versionning notes/TODOs etc.. outside git and want to have them appear in VSC
* Build the `package` target to automatically build and zip up your dll in a ready-to-distribute format.
