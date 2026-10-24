# mod-dungeon-quests

An AzerothCore module (WotLK 3.3.5a) that puts a neutral quest giver, the **Quest Emissary**, at the entrance and at the end of every 5-player dungeon. It gives and takes back the dungeon's quests, whatever the race, the faction and the quest chain.

## Key features

- Every 5-player dungeon of Classic, The Burning Crusade and Wrath of the Lich King.
- Alliance and Horde quests open to both factions, without the quests that lead to them.
- The game's own quest windows and quest markers, with pages for long lists.
- Still checked: dailies and weeklies, holidays, mutually exclusive quests, room in the log.
- No player data kept.

## Dependencies

| Dependency | For |
|---|---|
| [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk) (WotLK 3.3.5a) | the server |

## How to install

1. Download `installer.exe` from the [WoW-mods-installer releases](https://github.com/Adrestie/WoW-mods-installer/releases).
2. Stop the worldserver.
3. Run `installer.exe`, give it this folder and the folders it asks for, click **Check**, then **Install**.
4. Rebuild the worldserver. On its next start, the core applies the module's SQL.

## How to update

Same steps with the new version's folder, choosing **Update** instead of **Install**. Your `mod-dungeon-quests.conf` settings are kept. Rebuild the worldserver.

## How to uninstall

Same steps, choosing **Remove**. Everything goes, database included. Rebuild the worldserver.

## Credits

- Inspired by the Dungeon-Quest-Pet module (its "Bryten Helper"), which gives many of the entrance spots.

## Configuration

`mod-dungeon-quests.conf` sets:

- the module on or off;
- whether class quests stay for their class, and quests above the character's level stay hidden;
- how many quests one page of the list shows.
