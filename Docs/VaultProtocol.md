# Vault Protocol

The vault protocol serves as a shared inventory buffer, where anyone can dump some of their excess items and thereby making them available for other players to use

## V 0.0.1 Alpha, its currently under discussion ands improves can still be made

### Its not Gifting
The Vault protocol and the Gifting protocol share a similar goal where you can make some of you items available to other players but both protocols achieve this in their own way with their own pro's and cons

|                          | Vault                        | Gifting                                | 
| ------------------------ | ---------------------------- | -------------------------------------- |
| Requires specific target | No                           | Yes                                    |
| Throughput               | Unlimited                    | Useably only a few gifts at at time    |
| Cross game compatibility | Limited, based on item names | Lots, items are described using traits |
| Can reject items         | No                           | Yes                                    |
| Works in asyncs          | Yes                          | Yes                                    |

### Two different type of Vaults
There is one global vault accessible for anyone, in addition to that some games can have personal vaults. Any game can add items to any vault, but they should only take items from their own personal vault or the global vault, the global vault and personal vaults use a similar data storage keys formatted like `V<team>:<slot>` for a list of all known items and then `V<team>:<slot>:<item name>` for the available quantity of a specific item, for the global vault, the slot number is always `0`

## Getting a list of items available in the value
First you will need to query a list of items that are known to the vault, this is stored in `V<team>:<slot>`, if your game supports a personal vault, then be sure to check both so you would send an AP Get for like for example:

```json
// (In this example your Team = 0, your Slot = 3)
{"cmd": "Get", "keys": ["V1:0", "V1:3"]}
```
