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

## Details on the implementation
- As the quest logs are normally read from localized string, I had to hook the UI method that searches for them on mouse hover.
- The response text for dialog topics are read from the `.esp` using the `fileOffset` attribute. I wasn't able to locate/trigger the method that would create and store the responses for my runtime-generated dialog topics and thus I went on a workaround that dynamically changes the strings of pre-defined template topics from `SQGLib.esp` using an event sink and a hook. To make it work seemlessly I also had to hook another method to manually set/unset the "yet said" flag on theses template topics. This comes with the annoying limitation of only 4 dialog topics at a given level of the custom dialog tree (I could have added more but I chose to be on-par with FO4). There's also support for hello topics and one single force greet topic by NPC.
- I added a runtime-compiled script system for quest, topic and package fragments, as well as aliases script. It uses a [forked version](https://github.com/ZeCroque/Caprica) of [Orvid's Caprica compiler](https://github.com/Orvid/Caprica) and does unfortunately a lot of memory leaks (AFAIK it's the case even on the release version of Caprica, so I'll reach out to Orvid about that). It tries to read unzipped vanilla scripts and flags (from either `Scripts/Source` or `Source/Scripts`) and then is able to compile source strings and store them into a custom script store that will provide vanilla callbacks the compiled scripts when needed. Currently in the sample project these scripts are read from `Data/SKSE/SQGLibSample` but the goal is obviously to generate them on the go with the procedural engine.
- I implemented a custom "fragment" system using event sinks and it is currently expecting to find :
    - A script named equally to the quest editorId for quest stages "fragments"
    - A script named `"TF_" + quest editorId` for dialog topics "fragments"
    - A script named equally to what's been given to `AddAliasPackages` for package "fragments"
- The generated quests, dialogs, packages and scripts are serialized using a [forked version](https://github.com/ZeCroque/DPF-Dynamic-Persistent-Forms) of [Dynamic Persistent Form](https://github.com/Thiago099/DPF-Dynamic-Persistent-Forms) (by Thiago099) into a new co-save with the `.sqg` extension to be restored when you load a savegame. I had to create a new co-save instead of using the `.skse` one because the SKSE's `LoadCallback` triggers too late in the savegame loading process and therefore the generated quest weren't inialized properly and didn't appear in the quest log. The savegame folder is found using a function copied from `SKSE` that is not visible from `CommonLibSSE`, so that's not ideal as I will have to update it whenever `SKSE` gets updated.

## Tips
* You can add the environment variable `MO2Path` to automatically add a debug target that will launch MO2 from Visual Studio. The debug target runs [DebugWithMO2.ps1](cmake/DebugWithMO2.ps1), which starts MO2, waits for `SkyrimSE.exe` to have SKSE injected, then attaches Visual Studio to it (attaching earlier makes the SKSE loader fail with "Couldn't inject DLL"). Stopping the debugger ("Stop Debugging") also closes the game, while "Detach All" leaves it running. Child process debugging is not needed. Since the attach happens after SKSE starts, early breakpoints (e.g. in `SKSEPluginLoad`) may be missed: wait for the debugger there (`while (!IsDebuggerPresent()) Sleep(100);`) if needed
	* Set `MO2_INSTANCE` to the name of the instance you want MO2 to manage (default is `Skyrim Special Edition`)
	* Set `MO2_TARGET` to the name of the target you want MO2 to run (default is `Skyrim Special Edition`). It can point to the SKSE loader
	* Set `MO2_PROFILE` to the name of the MO2 profile you want to use (useful to have MO2 write to a mod folder instead of the overwrite in the chosen profile)
* Set `LEGACY_SCRIPT_PATH` to `ON` to use the old Scripts/Source path instead of the new (and stupid) Source/Scripts path
* Set `NOTES_FOLDER_PATH` if you are versionning notes/TODOs etc.. outside git and want to have them appear in VSC
* Build the `package` target to automatically build and zip up your dll in a ready-to-distribute format.
