# ideaLauncher — LightEdit Launcher for JetBrains IDEs (Windows)

This repository provides a tiny Windows launcher (ideaLauncher.exe) and ready-to-use configuration to run a JetBrains
IDE in LightEdit mode. You can use IntelliJ IDEA, PyCharm, or another JetBrains IDE build, as long as you adjust the
configuration accordingly.

The guide below explains how to build the launcher and assemble the deploy folder that can be copied to `C:/Program Files/JetBrains/LightEdit`.

## 1) Build the launcher (ideaLauncher.exe)

If you use CLion or CMake:

- Target name: ideaLauncher (executable)
- Recommended profile: Release

Steps (from CLion or terminal with the configured CMake profile):

1. Build target "ideaLauncher" in Release.
2. The produced executable will be copied to the `deploy` directory.
3. During the build, the following helper files are also copied into `deploy` (if they don't already exist):
   - `deploy/install_open_idea_lightedit.bat`
   - `deploy/uninstall_open_idea_lightedit.bat`
   - `deploy/update-hash.ps1`
   - `deploy/data/idea.properties`
   - `deploy/data/idea.vmoptions`
   - `deploy/data/launcher.yaml`

## 2) Finish the `deploy` directory

We will create a self-contained folder that looks like this when finished:

```plaintext
C:/Program Files/JetBrains/LightEdit
├──app
│  ├──bin
│  ├── ...other directories from the IDE .zip
│  └  product-info.json
├──data
│  ├  idea.properties
│  ├  idea.vmoptions
│  └  launcher.yaml
├  ideaLauncher.exe
├  install_open_idea_lightedit.bat
├  libgcc_s_seh-1.dll
├  libstdc++-6.dll
├  libwinpthread-1.dll
├  uninstall_open_idea_lightedit.bat
└  update-hash.ps1
```

You can assemble this layout inside the repository first as the `deploy` directory and then copy it to C:/Program
Files/JetBrains/LightEdit.

## 3) Scripts and data copied during build

The CMake build copies the following files into `deploy` if they do not already exist:

- deploy/install_open_idea_lightedit.bat
- deploy/uninstall_open_idea_lightedit.bat
- deploy/update-hash.ps1
- deploy/data/idea.properties
- deploy/data/idea.vmoptions
- deploy/data/launcher.yaml

If any of these are missing (e.g., if you cleaned the folder manually), rebuild the target or copy them from `files_to_copy` in the repository.

## 4) Provide the runtime MinGW DLLs

From your CLion installation (or another JetBrains IDE that ships MinGW), copy these DLLs to the root of `deploy`:

- libgcc_s_seh-1.dll
- libstdc++-6.dll
- libwinpthread-1.dll

Typical source path (example):
`C:/Program Files/JetBrains/CLion <version>/bin/mingw/bin`

Place them directly in `deploy`, next to ideaLauncher.exe.

## 5) Put an IDE under `deploy/app`

Download a Windows ZIP distribution of a JetBrains IDE. For IntelliJ IDEA:

- https://www.jetbrains.com/idea/download/download-thanks.html?platform=windowsZip

Notes:

- You can also use PyCharm or another IDE. If it’s not IntelliJ IDEA, you must update the product name in launcher.yaml
  accordingly (see next section).
- Unzip the downloaded archive into `deploy/app` so that `deploy/app` contains bin, lib, jbr, plugins, etc.

## 6) Configure data files

In `deploy/data` you should have:

- `idea.properties`
- `idea.vmoptions`
- `launcher.yaml`

Review `idea.properties` and `idea.vmoptions` and adjust as needed. You can compare these with the corresponding files
in `deploy/app/bin` if desired.

`launcher.yaml`:

- Ensure the product ID/name inside matches the IDE you placed under `app` (IntelliJ IDEA vs. PyCharm, etc.).
- Run `update-hash.ps1` to recompute the SHA in `launcher.yaml` after any change to files that are hashed by
  the script.

Example (run PowerShell from `deploy`):
```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
./update-hash.ps1
```

## 7) Main executable: ideaLauncher.exe

This executable is the entry point used to start the IDE in LightEdit mode.

## 8) Install or copy to Program Files

When deploy is complete, copy the whole `deploy` folder to:
`C:/Program Files/JetBrains/LightEdit`

Optionally, you can add the "Open with IntelliJ LightEdit" right-click context menu entry using:

- `C:/Program Files/JetBrains/LightEdit/install_open_idea_lightedit.bat`
  To remove, use:
- `C:/Program Files/JetBrains/LightEdit/uninstall_open_idea_lightedit.bat`

## 9) Quick checklist

- [ ] `deploy` contains `ideaLauncher.exe` - compilation result
- [ ] `deploy/data` contains `idea.properties`, `idea.vmoptions`, `launcher.yaml`
- [ ] `launcher.yaml` adjusted to your IDE and updated according to `update-hash.ps1`
- [ ] `deploy/app` contains the unzipped IDE (bin, lib, jbr, plugins, ...)
- [ ] `deploy` contains MinGW DLLs: `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`
- [ ] Optional: `run install_open_idea_lightedit.bat` as admin after copying to Program Files

## 10) First run

- Register if you have a license.
- Disable plugins not required for LightEdit (this can significantly improve performance).
- Add your favorite editor plugins.
- Consider adding shortcut `Ctrl-Q` to close the IDE (`File/Exit` action.
- Enjoy! :)

## Notes

- This setup is intended to run the IDE in LightEdit mode, optimized for quick file editing without loading full
  projects.
- If you change the IDE build under app or adjust data files, rerun update-hash.ps1 to keep launcher.yaml in sync.
