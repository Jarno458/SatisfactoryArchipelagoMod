Satisfactory 1.1 support. Incompatible with previous worlds. Post-goal progression content no longer placed in future milestones. New goals: Consistent AWESOME Points per Minute, Exploration




## Critical Changes

* **Completely broke compatibility with all saves from 1.0 or games generated with apworld 1.0.3** (see below for why)
  * To keep playing a previous world, don't update the mod.
* Altered the way the mod handles progression.
  * `final_elevator_tier` goal option now determines what milestones tiers exist
  * Progression items from other slots in the multiworld are no longer placed in milestone tiers past your goal
    This means you no longer *have* to release your slot when playing with multiple slots.

## New Stuff

* New goal: Exploration
  * Submit items you've found in the world to a T1 Hub milestone to complete the goal.
* New goal: AWESOME Sink points per minute, maintained over a period of time
  * Maintain a consistent level of AWESOME Sink points per minute to complete the goal. Dipping below the required points per minute will reset the timer.
  * AWESOME Sink UI includes a threshold bar displaying the level and a timer displaying how long you have maintained the required points per minute.
* New option: Randomize starting recipes
  * You can randomize the initial recipes that are available to you. This also opens up additional options for initial Iron Ingot, plate, etc. recipes.
  * Recipes are made hand-craftable as needed even if they normally aren't in vanilla (ex. Copper Alloy Ingot).
* New option: milestone cost multiplier
* Added ability to hint the next recipe in Archipelago logic for a part with `!hint part_name`
  * Previous hinting functionality still exists via hinting for a specific recipe with `!hint Recipe: recipe_name`
* Added 1.1 content to the randomizer pool
* Added `Single:` items for parts, and renamed all equipment bundles to `Single:` from `Bundle:` since they only contain one item
* Parts can now be purchased in the AWESOME shop as you obtain recipes that produce them
* Total points required for the AWESOME Sink total points goal is now displayed in the AWESOME Sink UI

APWorld:

* Added option groups and presets to options page (currently only accessible via self-hosting the Archipelago website)
* Added support for Universal Tracker

## Changed Stuff

* The option to randomize starting recipes allows playing through the game's tutorial.
  This alters the tutorial Hub tiers as needed to accommodate randomized recipes.
* Lowered build costs for the Assembler and Foundry to increase the variety of recipes that could be picked at the start of the game
* Improve websocket security
* Moved connection info input from Mod Configs to Mod Savegame Settings (session settings)
  * Now entered as you create the world
* Temporarily removed [MAM Enhancer](https://ficsit.app/mod/MAMTips) dependency to get this update out sooner
  * This means unlock info in the MAM is no longer displayed correctly, but it will be added back in a future update

## Fixed Stuff

* Greatly optimized apworld generation speed
* Added missing Recipes for (Iron Pipe, Biocoal, Charcoal, Sloppy Alumina, Hoverpack, Jetpack, Nobelisk Detonator)
* Fixed warning about uncompressed websockets
* Potentionally fixed deathlink triggering twice with thanks to @inkblots
* Removed bugged item ids for concrete and metallic walls, they are still available as customizations
* Fixed typo in `Building: Frame foundation`
* Re-added some missing cosmetic items in AWESOME shop
* Added missing Bundle for Dark Matter Crystals
