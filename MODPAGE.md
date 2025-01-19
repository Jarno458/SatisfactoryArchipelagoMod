# Archipelago Multi-World Randomizer

Experience Satisfactory in a whole new way by randomizing your game progression, potentially bringing completely separate video games into the mix too!

This mod is a [randomizer](https://tvtropes.org/pmwiki/pmwiki.php/Main/VideoGameRandomizer),
meaning it modifies your path of progression through the game,
forcing you to employ different approaches to reach certain goals.
The randomizer's logic system ensures that the game can still be finished, but the path to get there is different every time.

The mod will shuffle around all things you unlock in the HUB, MAM, and even add a few new unlocks to the AWESOME Shop.
You will have to look through all options available to you to find what to unlock next.
For example, if you need an assembler to craft something but cant find where to get access to it,
you could try to unlock some more nodes in the MAM as it might be hiding there.

[![Randomized](https://raw.githubusercontent.com/Jarno458/SatisfactoryArchipelagoMod/main/Docs/random.jpg)](https://raw.githubusercontent.com/Jarno458/SatisfactoryArchipelagoMod/main/Docs/random.jpg)

The Archipelago randomizer allows progression from over 60 different games to be mixed,
this is called a multi-world randomizer.

## Notable Features

### Archipelago Portal

Use Archipelago Portals to send and receive items from other compatible Archipelago games.
Send your friends your Nuclear Waste, send coffee mugs to Stardew Valley players, and more!

### EnergyLink

Send and receive power from any compatible Archipelago game, such as other Satisfactory worlds, Factorio, or even Stardew Valley.

Simply build a Power Storage building and hook it up to your power grid.
Remember that Biomass Burners can't charge Power Storage buildings, the same as base game.

## How to Setup

For simplicity's sake, this guide focuses on setting up a single player experience.
For more complex games with more players feel free to visit [archipelago.gg](https://archipelago.gg/) and/or the [Archipelago Discord](https://discord.gg/archipelago).

Before you can start using the mod you will need to generate a randomized game using Archipelago.

<!-- TODO remove me once local gen no longer needed -->
**Since this mod is still in active development, the convenient Player Options page is not yet available on the Archipelago website for Satisfactory.**

You will need to download the Archipelago Client and load a custom apworld file to generate a game.
If you get stuck, ask for help in the [Archipelago Discord](https://discord.gg/archipelago).

1. Download the Archipelago Client installer from the [Archipelago GitHub Releases](https://github.com/ArchipelagoMW/Archipelago/releases/latest)
2. Run the installer
3. Open the Archipelago Launcher
4. Download the latest released apworld file from [here](https://github.com/Jarno458/SatisfactoryArchipelagoMod/releases)
5. Press the "Browse Files" button
6. Place the downloaded apworld file into the `/lib/worlds` folder that the button took you to
7. Close and reopen the Archipelago Launcher
8. Press the "Generate Template Settings" button
9. In the folder that pressing the button took you to, make a copy of the `Satisfactory.yaml` file and open it in a text editor
10. Edit your a player settings yaml file by following the guide here: <https://archipelago.gg/tutorial/Archipelago/advanced_settings/en>
11. Move your copy of the file to the `/Players` folder
12. Return to the Archipelago Launcher and hit the `Generate` button
13. Use the zip in the `/Output` folder for the next steps below.

<!-- TODO remove me once local gen no longer needed -->
<!-- You will also need to generate a game yourself by [downloading `Satisfactory` branch of the server-side code](https://github.com/Jarno458/Archipelago/tree/Satisfactory) and setting up the python files required to generate a game. -->

<!-- 
TODO once player options page is available:
Before you start you will need to generate a randomized game,
this is done on the [player options](https://archipelago.gg/games/Satisfactory/player-options) page,
on the options page you select the settings for your game or leave them as default.

You need to change the `Player Name` to whatever name you like to called as.
Once you are done configuring your game click on the `Generate Game` button.
Generation might take a few seconds, after that you will be redirect to a page showing your `Seed Info`, on this page click on `Create New Room`.

[![Setup1](https://raw.githubusercontent.com/Jarno458/SatisfactoryArchipelagoMod/main/Docs/Setup1.JPG)](https://raw.githubusercontent.com/Jarno458/SatisfactoryArchipelagoMod/main/Docs/Setup1.JPG)

-->

<!-- TODO remove me once local gen no longer needed -->
Once you have generated a game locally you can upload it to the Archipelago website via the [Upload a Pre-Generated Game](https://archipelago.gg/uploads) page.

This will redirect you to a page where you can create a room from your upload; do so.
On the room page you will see connection details, for example, `/connect archipelago.gg:60282`.
Keep this room page open and bookmark the tab, otherwise, you will lose access to your room.

Install this mod via the Mod Manager if you haven't already and fire up your game.

Open the mod's config settings from the main menu - on the left, go to `Mods`, select `Archipelago` and in the connection settings enter the following:

- For URI, enter the connection URI as seen on the room page without the /connect part, so for example just `archipelago.gg:60282`
- For Username, choose the `Player Name` you selected on the options-page or when creating your yaml file.
- Enter a password if you set one earlier.

[![Setup2](https://raw.githubusercontent.com/Jarno458/SatisfactoryArchipelagoMod/main/Docs/Setup2.JPG)](https://raw.githubusercontent.com/Jarno458/SatisfactoryArchipelagoMod/main/Docs/Setup2.JPG)

Now start a new Satisfactory save as normal, but be sure to skip the intro tutorial.
Once you load into the game you should see chat messages indicating success or failure connecting to the server.

## How to Continue a Save

Playing with this mod requires a connection to an Archipelago server.
This server will automatically shut down your room after 2h of inactivity, and the port your room was previously hosted on may be reassigned while you're away.

Upon loading into your save file you will see a chat message indicating connection failure if the room is asleep, its port changed, (or if the connection failed for some other reason).

In order to continue your save, you will first need to restart the room by re-opening or re-loading the `Room Page` url you held onto earlier, for example `https://archipelago.gg/room/dQw4w9WgXcQ`, this will spin your room back up on the server.

Back in the game, load your save file again and the connection should succeed.
If it doesn't, your room's port may have been reassigned.

Check the connection details in the room. If the port has changed, go back to the mod config screen,
enter the new port in the URI field, and enable the `Force override settings in save` checkbox.

Once you've loaded into the save again after the port change, disable the `Force override settings in save` checkbox.

## Network Activity Transparency

In order to achieve the cross-game randomization functionality this mod communicates with an external server that you specify in the mod settings.
No default server connection is provided, so no network connection is made by default.
This external server could either be hosted by you or by the Archipelago servers depending on what you configure outside of the game.

Once connection details are entered, connection to the server will be attempted whenever you load any game save unless you uncheck the "Archipelago Enabled" option in the mod settings.
If the connection succeeds, the sever will transmit information to the game client that is used to generate schematics, items, recipes, etc. at runtime.
Unlocking schematics in the game and sending chat messages in the in-game text chat will send messages to the server to handle cross-game chat and progression tracking.
Messages can be sent from the server to the game client to display messages in the in-game text chat and to unlock schematics or trigger other actions in-game.

## Support the Developers

- Jarno: <https://ko-fi.com/P5P718V0J4>
- Robb: _If you enjoy my work, please consider donating to my [completely optional tip jar](https://ko-fi.com/robb4)._

## Copyright Notice

- Archipelago Logo: © 2022 by Krista Corkos and Christopher Wilson is licensed under Attribution-NonCommercial 4.0 International. To view a copy of this license, visit <http://creativecommons.org/licenses/by-nc/4.0/>
- See LICENSE files in the [source repository](https://github.com/Jarno458/SatisfactoryArchipelagoMod) for more detailed license information.
