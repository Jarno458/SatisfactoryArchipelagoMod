# Vault Protocol

The vault protocol serves as a shared inventory buffer, where anyone can dump some of their excess items and thereby making them available for other players to use

## V 0.0.1 Alpha, its currently under discussion ands improves can still be made

### Its not Gifting
The Vault protocol and the [Gifting API](<https://github.com/agilbert1412/Archipelago.Gifting.Net/blob/main/Documentation/Gifting%20API.md>) share a similar goal where you can make some of you items available to other players but both protocols achieve this in their own way with their own pro's and cons

|                          | Vault                        | Gifting                                | 
| ------------------------ | ---------------------------- | -------------------------------------- |
| Requires specific target | No                           | Yes                                    |
| Throughput               | Unlimited                    | Useably only a few gifts at at time    |
| Cross game compatibility | Limited, based on item names | Lots, items are described using traits |
| Can reject items         | No                           | Yes                                    |
| Works in asyncs          | Yes                          | Yes                                    |

### Two different type of Vaults
There is one global vault accessible for anyone, in addition to that some games can have personal vaults. Any game can add items to any vault, but they should only take items from their own personal vault or the global vault, the global vault and personal vaults use a similar data storage keys formatted like `V<team>:<slot>:<item name>` for the available quantity of a specific item, for the global vault, the slot number is always `0`

## Checking items available in the vaultfor taking
Since you can only consume items your game understands, its as simple as reading the keys for each item you care about, so you can build a single `Get` packet to read the amounts for all items you care about, in the example below we will at a `Bow`, `Arrow`, `Knife`, if you use a personal vault, its important to look for those items in both vaults, in the example below we assume your game does use a personal vault, item names are always lower case to prevent mismatching casings

```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Get", "keys": ["V1:0:bow", "V1:3:bow", "V1:0:arrow", "V1:3:arrow", "V1:0:knife", "V1:3:knife"]}
```

Your response will contain all keys and the amounts of how much of each item is available in the vault, be aware some keys will have a value of `null` if there never was an item with that name added to the vault

## Storing items to the global vault
Storing items to the vault is as simple as incrementing the key corresponding to the item name and vault, item names are in lowercase. if you plan to add multiple instances of the same item to the same vault, its best to cache them locally and only send them to the server in bulk to preserve bandwidth, So if we want to add a shotgun to the global vault we would do it like this:

```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Set", "key": "V1:0:shotgun", "operations": [{"operation": "add", "value": 1}]}
```

We don't have to specify any default, as the default value will default to `0` anyway

## Taking items from the vault
Taking items to the vault is a little bit more tricky as we need to wait for the `SetReply` to tell us how many items we successfully managed to take out of the vault. we also need to add a 2nd operation to make sure the amount of items in the vault can never go negative, so if we want to take 10 balloons out of our personal vault it would go something like this

```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Set", "key": "V1:3:balloon", "want_reply": true, "operations": [{"operation": "add", "value": -10}, {"operation": "max", "value": 0}]}
```
now we listen for the server its `SetReply` to our `Set` packet to tell us how many we got, for example a reply could look like this
```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "SetReply", "key": "V1:3:balloon", "value": 0, "original_value": 7, "slot": 3}
```
In the example above, there where only 7 balloons in the vault, so by subtracting `value` from `original_value` we can see how many we managed to obtain

## Personal vault
### Setting up item names
If you want to support a personal vault for your game, we should first let other other senders know what items they can put into it, we do this by storing a list of items per game that that game can accept,
The items are stored under a game specific key in the format of `V<game>` (so for V6 it will be VVVVVVV). you can find the game that belongs to specific slot in the `slot_info` you received in the `Connected` packet, The value should be a dictionary for lowercase item names and their values can be null if you just want item to be added based on its item name. Note this value should not change during game play as your game can not all of a sudden handle new items

```json
{"cmd": "Set", "key": "VTimespinner", "operations": [{"operation": "replace", "value": { 
    "blood orb": null,
    "iron orb": null,
    "ether": null,
}}]}
```

### Storing items to the an personal vault
Storing items to a personal vault is the same as the global vault, however it makes no sense to store items to a personal vault that the game of that slot does not know how to process. Therefor the before adding anything to a personal vault, we first need to check what the target game can receive, for this we can look at game specific key that holds all item names that game can accept, its formatted like `V<game>`

```json
// Get supported items by the game
{"cmd": "Get", "keys": ["VTimespinner"]}
```
This should return a dictionary with as keys the item names that game can handle, if this returns null it means that target game does not expose what item names you can store, in such cases its best not use the personal vault

## Cross game item sharing
### Setting up trait information on your items
If you want to take items from other games for cross game item sharing the game specific item list should be extended with traits, traits allow the sending game to decide a good enough match for an item in their game to add it to vault under a different item name that your game understands for simplicity the traits follow the same guidelines are those for the [Gifting API](<https://github.com/agilbert1412/Archipelago.Gifting.Net/blob/main/Documentation/Gifting%20API.md#gifttrait-specification>) so games don't have to maintain duplicate lists

The items are stored under a game specific key in the format of `V<game>` (so for V6 it will be VVVVVVV). you can find the game that belongs to specific slot in the `slot_info` you received in the `Connected` packet, The value should be a dictionary for lowercase item names and their values can be null if you just want item to be added based on its item name, but to specify traits for an item, its value should be a dictionary with the traits as keys, and an array of floats as its value per trait, the first value in the array will be the traits `quality`, the 2nd value will be the traits its `duration`. If duration is 1.0 it can be omitted. if both quality and duration are 1.0 then they can both be omitted leaving the array empty, in this case the array can be replaced with null. Items don't change their traits, so this dictionary is supposed to be static data that can be cached by other clients


```json
{"cmd": "Set", "key": "VSatisfactory", "operations": [{"operation": "replace", "value": { 
    "iron ore": {"Iron": [0.856, 0.77], "Ore": [0.856], "Mineral": null},
    "golden nut statue": {"Gold": [2.56]},
    "medical inhaler": {"Healing": [1.0, 0.5]},
}}]}
```

### Taking
Just like taking items normally, your game should only take items from vaults that you know, so you can just check the vaults (global and personal if applicable) for item names you understand, the responsibility lies with the client that stores the item to store it under an item name your game understands

### Storing
If you want to store an item to be used by a different game you will have to store the item under and item name that the desired target game can handle, for this you should look at the game specific item list under the key `V<game>` to get a dictionary of all item names that that game can handle, then next you can look at the traits of each item if they are defined, if they are you can try make an educated guess on what item would be the best match in the target game. The simplest way to find the best matching item is to define traits for your own game item and then find the item in the target get with the most matching traits, and if there is a tie for multiple items, check each trait's `quality` and `duration` for the item that has the least deviation from your own game item. After you decided on the item name the item should de stored as then simply store it into the vault as any other item. So it would do something like this
```json
// Get supported items by the game
{"cmd": "Get", "keys": ["VTimespinner"]}
```
The response should not change over the course of the gameplay, so the value should be cached, decide under what item name you want to store your game its item, for example Timespinner could decide to add an `ether` as a `mana potion`. Then store the item like normal 
```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Set", "key": "V1:3:mana potion", "operations": [{"operation": "add", "value": 1}]}
```