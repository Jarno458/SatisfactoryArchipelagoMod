# Vault Protocol

The vault protocol serves as a shared inventory buffer, where anyone can dump some of their excess items and thereby making them available for other players to use

## V 0.1 Beta, its currently under discussion ands improves can still be made

### Its not Gifting
The Vault protocol and the [Gifting API](<https://github.com/agilbert1412/Archipelago.Gifting.Net/blob/main/Documentation/Gifting%20API.md>) share a similar goal where you can make some of you items available to other players but both protocols achieve this in their own way with their own pro's and cons

|                          | Vault                               | Gifting                                       | 
| ------------------------ | ----------------------------------- | --------------------------------------------- |
| Requires specific target | No                                  | Yes                                           |
| Throughput               | Unlimited                           | Useably only a few gifts at at time           |
| Cross game compatibility | Limited, mostly based on item names | Lots, items are always described using traits |
| Can reject items         | No                                  | Yes                                           |
| Works in asyncs          | Yes                                 | Yes                                           |

### Two different type of Vaults
There is one global vault accessible for anyone, in addition to that some games can have personal vaults.  
Any game can add items to any vault, but they should only take items from their own personal vault or the global vault, the global vault and personal vaults use a similar data storage keys formatted like `V<team>:<slot>:<item name>` for the available quantity of a specific item.  
For the global vault the slot number is always `0`.

### Three levels of integrating with the Vault Protocol
There are 3 levels to choose from when implementing the vault protocol, higher levels allow for better cross game compatibility but also require more effort and setup, they are as follows:
* Level 1 (Basic)
  * Global Vault only, the game does not support any personaly vaults and will only take and store items in the global vault based on item names
* Level 2 (Personal)
  * The game support a personal vault, and offers a list of valid items names for other games to store in thier personal vault
* Level 3 (Personal vault with item traits)
  * The game support a personal vault, and offers a list of trait values per item name so other games can decide under which name to store items that dont simply match on name 

## Checking items available in the vaultfor taking
Since you can only consume items your game understands, its as simple as reading the keys for each item you care about.  
You can build a single `Get` packet to read the amounts for all items you care about, in the example below we will at a `Bow`, `Arrow`, `Knife`.  
If you use a personal vault, its important to look for those items in both vaults, in the example below we assume your game does use a personal vault.  
Item names are always lower case to prevent mismatching casings
```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Get", "keys": ["V1:0:bow", "V1:3:bow", "V1:0:arrow", "V1:3:arrow", "V1:0:knife", "V1:3:knife"]}
```
Your response will contain all keys and the amounts of how much of each item is available in the vault.
Be aware some keys will have a value of `null` if there never was an item with that name added to the vault.

## Global vault (level 1)
### Storing items to a vault
Storing items to the vault is as simple as incrementing the key corresponding to the item name and vault.  
Make sure the value never exceeds an 64 bit interger (9223372036854775807).  
Item names are in lowercase.  
If you plan to add multiple instances of the same item to the same vault, its best to cache them locally and only send them to the server in bulk to preserve bandwidth.  
So if we want to add a 4 shotgun to the global vault we would do it like this:
```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Set", "key": "V1:0:shotgun", "operations": [{"operation": "add", "value": 4}, {"operation": "min", "value": 9223372036854775807}]}
```
We don't have to specify any default, as the default value will default to `0` anyway.

### Taking items from a vault
Taking items to the vault is a little bit more tricky as we need to wait for the `SetReply` to tell us how many items we successfully managed to take out of the vault.  
We also need to add a 2nd operation to make sure the amount of items in the vault can never go negative, so if we want to take 10 balloons out of our personal vault it would go something like this:
```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Set", "key": "V1:3:balloon", "want_reply": true, "operations": [{"operation": "add", "value": -10}, {"operation": "max", "value": 0}]}
```
Now we listen for the server its `SetReply` to our `Set` packet to tell us how many we got, for example a reply could look like this:
```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "SetReply", "key": "V1:3:balloon", "value": 0, "original_value": 7, "slot": 3}
```
In the example above, there where only 7 balloons in the vault, so by subtracting `value` from `original_value` we can see how many we managed to obtain

# Personal Vaults
Personal vaults are used to send store items specifically for a specific slot, therefor only items that the game corresponding to the slot can use should be stored there.  
If you want to support a personal vault for your game, we should first let other other senders know what items they can put into it. We do this by storing an a list items per game that that game can accept.
The items are stored under a game specific key in the format of `V<game>` (so for V6 it will be VVVVVVV).  
You can find the game that belongs to specific slot in the `slot_info` you received in the `Connected` packet.  

## Checking what games support personal vaults
When looking if a game supports personal vaults, requests the personal vault keys for the games that are currently in the multiworld (you can likely skip your own game as you should know the details about your own game already).  
This information should not change during game play as your game can not all of a sudden handle new items.  
This information is game wide, the vault protocol does now allow for specific slots to make thier own decisions.  
```json
// (In this example the following games are part of this multiworld Satisfactory, Timespinner, VVVVVV)
{"cmd": "Get", "keys": ["VSatisfactory", "VTimespinner", "VVVVVVV"]}
```
The responses per game can vary a lot depending on the game its integration level.  
Level 1: The retrieved value is `null` then that game does not support personal vault  
Level 2: The retrieved value is an array and it should contain a list of item names it can accept into its personal vault  
Level 3: The retrieved value is an object and its keys should be item names it can accept into its personal vault and the values should contain the item its corresponding gifting traits  
Lets go over a little responses below:
```json
// (In this example the following games are part of this multiworld Satisfactory, Timespinner, VVVVVV)
{"cmd": "Retrieved", "keys": {
    "VSatisfactory": {"medical inhaler": {"Healing": [1.0, 0.5]}},
    "VTimespinner": ["blood orb", "iron orb", "ether"],
    "VVVVVVV": null
}}
```

## Setting up personal vault (Level 2)
The simplest form of supporting a personal vault is by providing an array of item names.  
The item namnes array is stored in the data storage user a key named with your game prefixed by a V.  
The value should be an array with lowercase item names.  
Note this value should not change during game play as your game can not all of a sudden handle new items.  
Here is an example for how this could look for Timespinner:
```json
// Timespinner can have "blood orb", "iron orb" and "ether" placed in thier personal vaults
{"cmd": "Set", "key": "VTimespinner", "operations": [{"operation": "replace", "value": ["blood orb", "iron orb", "ether"]}]}
```

## Setting up personal vault with item traits (Level 3)
The more complete and complex way of supporting a personal vault with item traits for cross game compatability requires you to setup gifting item trait per item.  
Traits allow the sending game to decide a good enough match for an item in their game to add it to your personal vault under a different item name that your game understands. For simplicity the traits follow the same guidelines are those for the [Gifting API](<https://github.com/agilbert1412/Archipelago.Gifting.Net/blob/main/Documentation/Gifting%20API.md#gifttrait-specification>) so games don't have to maintain duplicate lists.  
This is done by storing and object with the lowercase item names as keys and item traits as values.  
The item namnes object is stored in the data storage under a key named with your game prefixed by a V.  
The object should be a dictionary for lowercase item names, to specify traits for an item, its value should be a dictionary with the traits as keys, and an array of floats as its value per trait, the first value in the array will be the traits `quality`, the 2nd value will be the traits its `duration`. If duration is 1.0 it can be omitted. if both quality and duration are 1.0 then they can both be omitted, in this case the array can be replaced with null.  
Items don't change their traits, so this dictionary is supposed to be static data that can be cached by other clients  
Here is an example for how this setup could look for Satisfactory:
```json
{"cmd": "Set", "key": "VSatisfactory", "operations": [{"operation": "replace", "value": { 
    "iron ore": {"Iron": [0.856, 0.77], "Ore": [0.856], "Mineral": null},
    "golden nut statue": {"Gold": [2.56]},
    "medical inhaler": {"Healing": [1.0, 0.5]},
}}]}
```

## Taking items from an personal vault
You should only ever taken items from your own personal vault, or the global vault.  
Taking items from your personal vault works just like taking items normally from the global vault.  
Keep in mind to never make the value go below 0
Your game should only take items that you know, so you can just check the vaults (global and personal if applicable) for item names you understand, the responsibility lies with the client that stores the item to store it under an item name your game understands.

## Storing items to the an personal vault
Storing items to a personal vault is the same as the global vault, however it makes no sense to store items to a personal vault that the game of that slot does not know how to process. 
Just like with global vault, make sure the value never exceeds an 64 bit interger (9223372036854775807).  
Therefor before adding anything to a personal vault, we first need to check what item names the target slots its game can receive as described abonve in [Checking what games support personal vaults](## Checking what games support personal vaults).
If the results is `null` that slot does not support a personal vault so you cant store an item there, maybe you can store it in global vault instead.  
If the result is an array, then make sure any item name you try to store in that personal vault as in that array as those are the only item names that slot can process.  
If the result is an object, then make sure any item name you try to store in that personal vault corresponds to an key on the object.  
If the result is an object ideally you should at look the traits of each item, try make an educated guess on what item would be the best match in the target game. The simplest way to find the best matching item is to define traits for your own game item and then find the item in the target get with the most matching traits, and if there is a tie for multiple items, check each trait's `quality` and `duration` for the item that has the least deviation from your own game item. 
