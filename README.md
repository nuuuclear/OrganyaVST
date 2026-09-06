## Organya VST

A VST instrument to replicate the sound of CaveStory's organya music format.

![screenshot](./.github/assets/screenshot.png)


## building
#### On windows:
```bash
./build-windows.bat release
```

if you try to build without the script, make sure you arn't using MinGW as JUCE does not support it.

#### On linux:
```bash
cmake --build --preset linux-gcc-release
```
or
```bash
cmake --build --preset linux-clang-release
```

#### On mac:
```bash
cmake --build --preset macos-release
```