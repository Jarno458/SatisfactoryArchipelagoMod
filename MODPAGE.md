# Archipelago Multi-World Randomizer

Coming Soon, current version is an very early alpha version that is tested internally in the Archipelago community

## Network Activity Transparency

In order to achieve the cross-game randomization functionality this mod communicates with an external server that you specify in the mod settings.
No default server connection is provided, so no network connection is made by default.
This external server could either be hosted by you or by the Archipelago servers depending on what you configure outside of the game.

Once connection details are entered, connection to the server will be attempted whenever you load any game save unless you uncheck the "Archipelago Enabled" option in the mod settings.
If the connection succeeds, the sever will transmit information to the game client that is used to generate schematics, items, recipes, etc. at runtime.
Unlocking schematics in the game and sending chat messages in the in-game text chat will send messages to the server to handle cross-game chat and progression tracking.
Messages can be sent from the server to the game client to display messages in the in-game text chat and to unlock schematics or trigger other actions in-game.

## Copyright Notice

- TODO APCpp?
- Some assets, including the Archipelago Logo: © 2022 by Krista Corkos and Christopher Wilson is licensed under Attribution-NonCommercial 4.0 International. To view a copy of this license, visit <http://creativecommons.org/licenses/by-nc/4.0/>
- See LICENSE files in the [source repository](https://github.com/Jarno458/SatisfactoryArchipelagoMod) for more detailed license information.
