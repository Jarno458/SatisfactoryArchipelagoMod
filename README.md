# SatisfactoryArchipelagoMod

Archipelago connection mod for [Satisfactory](https://www.satisfactorygame.com/).
Experience Satisfactory in a whole new way by randomizing your game progression, potentially bringing completely separate video games into the mix too!

Learn more about Archipelago [on the Archipelago website](https://archipelago.gg/).

Learn more about this mod by reading [the modpage on ficsit.app](https://ficsit.app/mod/Archipelago).

## Playing the Game

For users new to Archipelago,
follow the setup directions on [ficsit.app](https://ficsit.app/mod/Archipelago).

For experienced users, or those looking for more information on how Archipelago changes Satisfactory,
and how to configure the mod for multiplayer,
see the [Satisfactory Archipelago info page](https://github.com/Jarno458/Archipelago/blob/Satisfactory/worlds/satisfactory/docs/en_Satisfactory.md)
and [Satisfactory Archipelago setup guide](https://github.com/Jarno458/Archipelago/blob/Satisfactory/worlds/satisfactory/docs/setup_en.md).

<!-- TODO link to the AP docs on this instead once that is published? -->

## Get Support

Archipelago Discord: <https://discord.gg/8Z65BR2>

Satisfactory Modding Discord: <https://discord.ficsit.app>

## Development Setup

You should only need to follow the directions below if you want to contribute to developing the mod.

### Prerequisites

### Satisfactory Modding Development Environment

[Follow this setup guide](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/index.html)

You will need [this tutorial](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/ImportingAnotherMod.html) later to import mods' source to your project.

### Mod Dependencies

This mod depends on a few other Satisfactory mods.
You will need to import them into your project using the tutorial above before this mod can compile.

- [ContentLib](https://github.com/Nogg-aholic/ContentLib)
- [MAM Enhancer](https://github.com/Nogg-aholic/MAMTips)
- [Free Samples](https://github.com/budak7273/FreeSamples)
- [FixClientResourceSinkPoints](https://github.com/budak7273/FixClientResourceSinkPoints)

### APCpp

This mod uses a [APCpp](https://github.com/Jarno458/APCpp/tree/Satisfactory) to communicate with the AP server.

In order to get this to work this mod uses static compiled libraries from APCpp.
The libraries are included in this repo, but if you want to update them you can follow these steps:

To do a static compile of APCpp, add this line below line 9 in the `CMakeLists.txt` inside APCpp:

```cmake
add_library(APCpp-static STATIC Archipelago.cpp Archipelago.h)
```

To update the used APCpp version, build a STATIC library version then:

1. Copy all *.lib over from `APCpp\build\lib\Release` to `Archipelago\Source\APCpp\lib\Win64`
2. Copy `Archipelago.h` over to `APCpp` to `Archipelago\Source\APCpp\inc`

### Archipelago Server

You need to have a running Archipelago server to test the mod in-game.
You can self host one from source by following the directions
[in the Archipelago repo](https://github.com/ArchipelagoMW/Archipelago/blob/main/docs/running%20from%20source.md).
