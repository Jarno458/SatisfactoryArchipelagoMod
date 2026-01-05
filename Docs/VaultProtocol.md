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

## Checking items available in the vault
Since you can only consume items your game understands, its as simple as reading the keys for each item you care about, so you can build a single `Get` packet to read the amounts for all items you care about, in the example below we will at a `Bow`, `Arrow`, `Knife`, if you use a personal vault, its important to look for those items in both vaults, in the example below we assume your game does use a personal vault, item names are always lower case to prevent mismatching casings

```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Get", "keys": ["V1:0:bow", "V1:3:bow", "V1:0:arrow", "V1:3:arrow", "V1:0:knife", "V1:3:knife"]}
```

Your response will contain all keys and the amounts of how much of each item is available in the vault, be aware some keys will have a value of `null` if there never was an item with that name added to the vault

## Adding items to the global vault
Adding items to the vault is simple a incrementing the key corresponding to the item and vault again item names are in lowercase. if you plan to add multiple instances of the same item to the same vault, its best to cache them locally and only send them to the server in bulk to preserve bandwidth, So if we want to add a shotgun to the global vault we would do it like this:

```json
// (In this example your Team = 1, your Slot = 3, global vault Slot = 0)
{"cmd": "Set", "key": "V1:0:shotgun", "operations": [{"operation": "add", "value": 1}]}
```

We don't have to specify any default, as the default value will default to `0` anyway

## Removing items from the vault
Removing items to the vault is a little bit more tricky as we need to wait for the `SetReply` to tell us how many items we successfully managed to take out of the vault. we also need to add a 2nd operation to make sure the amount of items in the vault can never go negative, so if we want to take 10 balloons out of our personal vault it would go something like this

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

## Adding support for a personal vault
If you want to support a personal vault for your game, we should first let other other sernders know what items they can put into it, we do this by storing a list of items per game that that game can accept, For each of those items you should specify some traits, the traits allow sending games to transforms items to a simular item the receiving game would understand. for simplicity the traits follow the same guidelines are those for the [Gifting API](<https://github.com/agilbert1412/Archipelago.Gifting.Net/blob/main/Documentation/Gifting%20API.md#gifttrait-specification>) so games dont have to maintain duplicate lists, in the data storage we just add the thair values for 1 single item, so our item list per game should look like this

```json

```

## Adding items to the an personal vault

//TODO describe items / traits

Adding items to a personal vault is about the same as the global vault, however it makes no sense to add items to a personal vault the game of the slot does not know how to process. Therefor the before adding anything to a personal vault, we first need to check what the game can receive, for this we can look at game specific key that holds all item names that game can accept, its formatted like `V<game>`

```json

```
