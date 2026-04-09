# Using Development Versions of the Mod

This page is a work in progress and should be improved Eventually™.

If you get stuck, contact us on the [Archipelago Discord](https://discord.gg/archipelago).

---

Using development versions requires generating the game yourself locally.

You may also need to [use Satisfactory Mod Manager to opt into a specific prerelease version of the mod](https://docs.ficsit.app/satisfactory-modding/latest/ForUsers/SatisfactoryModManager.html#InstallSpecificModVersion).

You will need to download the Archipelago Client and load a custom apworld file to generate a game.
If you get stuck, ask for help in the [Archipelago Discord](https://discord.gg/archipelago).

1. Download the Archipelago Client installer from the [Archipelago GitHub Releases](https://github.com/ArchipelagoMW/Archipelago/releases/latest)
2. Run the installer
3. Download the relevant development apworld file from [the GitHub releases page](https://github.com/Jarno458/SatisfactoryArchipelagoMod/releases) (probably the latest one)
4. Open the Archipelago Launcher
5. Search for and Open the "Browse Files" task inside the Archipelago Launcher
6. TODO remove or comment out the existing Satisfactory apworld (is there a nice way to override this?)
7. Place the downloaded apworld file into the `/lib/worlds` folder that the button took you to (TODO can you just use double-click execute to copy it to `Archipelago/custom_worlds` in this case?)
8. Close and reopen the Archipelago Launcher
9. Search for and Open the "Generate Template Settings" task
10. In the folder that pressing the button took you to, make a copy of the `Satisfactory.yaml` file and open it in a text editor
11. Edit your a player settings yaml file by following the guide here: <https://archipelago.gg/tutorial/Archipelago/advanced_settings/en>
12. Move your copy of the file to the launcher's `/Players` folder
13. Return to the Archipelago Launcher and hit the `Generate` button
14. Use the zip in the `/Output` folder for the next steps below.

Once you have generated a game locally you can upload it to the Archipelago website via the [Upload a Pre-Generated Game](https://archipelago.gg/uploads) page.
