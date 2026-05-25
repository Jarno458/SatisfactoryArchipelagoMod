# SatisfactoryArchipelagoMod

Archipelago connection mod for [Satisfactory](https://www.satisfactorygame.com/).
Experience Satisfactory in a whole new way by randomizing your game progression, potentially bringing completely separate video games into the mix too!

Learn more about Archipelago [on the Archipelago website](https://archipelago.gg/).

Learn more about this mod by reading [the modpage on ficsit.app](https://ficsit.app/mod/Archipelago)
or the [Satisfactory Archipelago info page](https://archipelago.gg/games/Satisfactory/info/en).

## Playing the Game

For users new to Archipelago,
follow the setup directions on [ficsit.app](https://ficsit.app/mod/Archipelago).

For experienced users, or those looking for more information on how Archipelago changes Satisfactory,
and how to configure the mod for multiplayer,
see:

- [Satisfactory Archipelago info page](https://archipelago.gg/games/Satisfactory/info/en)
- [Satisfactory Archipelago setup guide](https://archipelago.gg/tutorial/Satisfactory/setup_en)

For working with versions of the Satisfactory apworld that haven't been merged into Archipelago core yet,
you can find the documentation on Jarno's fork of Archipelago, which is used for development.
Note that any Archipelago internal links will be broken there because you're viewing markdown files and not the deployed site.
You may also need to switch branches to see the version of the documentation you care about.

- [Jarno's Archipelago repository: en_Satisfactory.md](https://github.com/Jarno458/Archipelago/blob/Satisfactory/worlds/satisfactory/docs/en_Satisfactory.md)
- [Jarno Archipelago repository: setup_en.md](https://github.com/Jarno458/Archipelago/blob/Satisfactory/worlds/satisfactory/docs/setup_en.md)


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
- [Fix Client Resource Sink Points](https://github.com/budak7273/FixClientResourceSinkPoints)
- [Hover Pack Fuse Reminder](https://github.com/budak7273/HoverpackFuseReminder)

### APCpp

This mod uses a specific branch of [APCpp](https://github.com/Jarno458/APCpp/tree/Satisfactory) to communicate with the AP server.

To update the used APCpp version:

- clone the [branch](https://github.com/Jarno458/APCpp/tree/Satisfactory) of APCpp
- Create a folder `build`
- Enter the folder
- `cmake .. -DWIN32=1`
- `cmake --build . --config Release`

1. Copy all `*.lib` over from `APCpp\build\` to `Archipelago\Source\APCpp\lib\Win64` and subdirectories
2. Copy all `*.h` over from `APCpp` to `Archipelago\Source\APCpp\inc`

For linux, its not currently working

### Archipelago Server

You need to have a running Archipelago server to test the mod in-game.
You can self host one from source by following the directions
[in the Archipelago repo](https://github.com/ArchipelagoMW/Archipelago/blob/main/docs/running%20from%20source.md).
