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
* You can add the environment variable `MO2Path` to automatically add a debug target that will launch MO2 from Visual Studio. The debug target runs [DebugWithMO2.ps1](cmake/DebugWithMO2.ps1), which starts MO2, waits for `SkyrimSE.exe` to have SKSE injected, then attaches Visual Studio to it (attaching earlier makes the SKSE loader fail with "Couldn't inject DLL"). Stopping the debugger ("Stop Debugging") also closes the game, while "Detach All" leaves it running. Child process debugging is not needed. Since the attach happens after SKSE starts, early breakpoints (e.g. in `SKSEPluginLoad`) may be missed: wait for the debugger there (`while (!IsDebuggerPresent()) Sleep(100);`) if needed
	* Set `MO2_INSTANCE` to the name of the instance you want MO2 to manage (default is `Skyrim Special Edition`)
	* Set `MO2_TARGET` to the name of the target you want MO2 to run (default is `Skyrim Special Edition`). It can point to the SKSE loader
	* Set `MO2_PROFILE` to the name of the MO2 profile you want to use (useful to have MO2 write to a mod folder instead of the overwrite in the chosen profile)
* Set `LEGACY_SCRIPT_PATH` to `ON` to use the old Scripts/Source path instead of the new (and stupid) Source/Scripts path
* Set `NOTES_FOLDER_PATH` if you are versionning notes/TODOs etc.. outside git and want to have them appear in VSC
* Build the `package` target to automatically build and zip up your dll in a ready-to-distribute format.
