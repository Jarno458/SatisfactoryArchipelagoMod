# SatisfactoryArchipelagoMod

TODO: Write this later™

Learn more about Archipelago [here](https://archipelago.gg/).

## Installation

Install this mod via the Satisfactory Mod Manager found on [ficsit.app](https://ficsit.app/mod/Archipelago).

TODO link to the AP docs on this instead

## Get Support

TODO

<https://discord.gg/xkVJ73E>

## Development Setup

You should only need to follow these directions if you want to contribute to developing the mod.

### Prerequisites

### Satisfactory Modding Development Environment

[Follow this setup guide](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/index.html)

You will need [this tutorial](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/ImportingAnotherMod.html) later to import mods' source to your project.

### ContentLib

This mod depends on the [ContentLib Satisfactory mod](https://github.com/Nogg-aholic/ContentLib) for runtime content generation.
You will need to import it into your project using the tutorial above before this mod can compile.

### APCpp

This mod uses [APCpp](https://github.com/N00byKing/APCpp) to communicate with the AP server.

In order to get this to work this mod uses static compiled libraries from APCpp.
The libraries are included in this repo, but if you want to update them you can follow these steps:

To do a static compile of APCpp, add this line below line 9 in the `CMakeLists.txt` inside APCpp:

```cmake
add_library(APCpp-static STATIC Archipelago.cpp Archipelago.h)
```

To update the used APCpp version, build a STATIC library version then:

1. Copy all *.lib over from `APCpp\build\lib\Release` to `Archipelago\Source\APCpp\lib`
2. Copy `Archipelago.h` over to `APCpp` to `Archipelago\Source\APCpp\public`

### Archipelago Server

TODO
