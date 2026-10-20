# mod-dungeon-quests

An AzerothCore module for World of Warcraft 3.3.5a (Wrath of the Lich King).

A neutral quest giver, the **Quest Emissary**, stands at the entrance and at
the end of every 5-player dungeon (Classic, The Burning Crusade, Wrath of the
Lich King). It gives and takes back the quests that are done in its dungeon,
**whatever the race, the faction and the quest chain**: walk into the
dungeon, take its quests, clear it, and hand them in at the end. A Horde
player can take the Alliance quests of the Deadmines, and the other way
round, without having done the quests that lead to them.

## In game

- The Emissary opens the game's own quest windows: the list, the text of
  each quest, its rewards (with the choice of one when there are several).
- Quests already in the log come first (question mark), then the quests to
  take (exclamation mark). Once a quest is taken, the list comes back for
  the next one.
- A finished quest is handed in to the Emissary, at the entrance or at the
  end of the dungeon.
- A list too long for the window (the client shows 32 quests at most) is cut
  into pages, with "More quests..." and "Previous quests..." buttons.
- The Emissary carries the usual quest marker above its head and on the
  minimap: a yellow question mark when a quest can be handed in, an
  exclamation mark when one can be taken.

**Ignored:** the race (and so the faction), the previous and next quests of
the chain, breadcrumb quests, the profession skill, the reputation and the
conditions of the database.

**Still checked:** the quest is neither in the log nor already handed in;
daily and weekly quests once a day or a week; seasonal quests during their
holiday only; mutually exclusive quests (the Alliance and Horde versions of
one quest cannot both be done when the database links them); room in the
quest log; and, by default, the class and the minimum level (see the
configuration).

A quest handed in to the Emissary does not offer the next quest of its
chain, and the rest of the chain out in the world may no longer be
available. This is intended.

## The quests of each dungeon

The `mod_dungeon_quests` table (world database: `map`, `quest`, `comment`)
lists the quests of each dungeon. The map the Emissary stands on picks its
list, so one creature (entry 94000) serves every dungeon.

The table is written by `tools/generate_quest_list.py` from the world
database and the server's DBC files. A quest belongs to a dungeon when:

- its zone (`QuestSortID`) is the dungeon's zone, except the zones several
  dungeons share (Hellfire Citadel, Coilfang Reservoir, Tempest Keep,
  Auchindoun); or
- every one of its objectives is found on the dungeon's map only: the
  creatures to kill (their spawns, kill credits and heroic versions
  included), the objects to use, and the items to bring back (the creatures
  and chests that drop them).

Left out: quests without objectives (going to talk to someone), quests
nobody gives (except daily quests, which the core rotates) and unused
quests. The current list holds 339 quests in 50 dungeons, daily and heroic
quests included when they are done in the dungeon (the Black Morass has
none).

To adjust the list by hand, edit the table and type `.reload config`: the
module reads it again. Such edits are lost when the module is reinstalled or
updated; to keep them, change `data/sql/world/dungeon_quests_02_list.sql` (or
the rules of the script) before installing. To write the list again:

```
python tools/generate_quest_list.py --dbc "<server Data>\dbc" --database acore_world --mysql "<path to mysql.exe>" --password <password>
```

## Where the Emissaries stand

117 Emissaries, in `data/sql/world/dungeon_quests_03_spawns.sql` (guids
9400000-9400116):

- **at the entrance**, by the players' arrival point: one per wing for
  Scarlet Monastery, Dire Maul, Maraudon and Stratholme, and one at each of
  the two doors of Uldaman;
- **at the end**, beside the last boss of the dungeon (or of each wing), or
  beside the creature or chest that closes the dungeon when its last boss is
  summoned by a script. There is no end Emissary in the Oculus (its last
  boss is fought on drakes, in the air) nor in the Violet Hold (a single
  room, where the entrance Emissary is enough).

An Emissary settles on the floor when it appears, should its spot be a
little above or below it. Many entrance spots are those of the
Dungeon-Quest-Pet module (its "Bryten Helper"), which inspired this one.

## Installation

With [WoW-mods-installer](https://github.com/Adrestie/WoW-mods-installer), or
by hand:

1. Copy this folder into `modules/` of the AzerothCore sources.
2. Run CMake again (otherwise the new module is skipped), then build.
3. Copy `conf/mod-dungeon-quests.conf.dist` into the server's
   `configs/modules` folder, as `mod-dungeon-quests.conf`.
4. On start, the worldserver applies the files of `data/sql/world` to the
   world database (when `Updates.EnableDatabases` allows it; otherwise apply
   them yourself, in order).

The module adds nothing to the characters database: it keeps no player data.

## Configuration

| Setting | Default | Effect |
|---|---|---|
| `DungeonQuests.Enable` | 1 | 0: the Emissaries have nothing to offer. |
| `DungeonQuests.RespectClass` | 1 | 0: class quests are offered to every class. |
| `DungeonQuests.RespectLevel` | 1 | 0: quests are offered below their minimum level. |
| `DungeonQuests.PageSize` | 30 | How many quests one page of the list shows (32 at most). |

## How it works

The creature has no row in `creature_queststarter` or
`creature_questender`. The quest packets the client sends to it
(`CMSG_QUESTGIVER_QUERY_QUEST`, `ACCEPT_QUEST`, `COMPLETE_QUEST`,
`REQUEST_REWARD`, `CHOOSE_REWARD`) are answered by the module before the core
handlers (the `CanPacketReceive` hook, in the same thread as those handlers),
with its own checks in place of `Player::CanTakeQuest`. The list is a gossip
menu (`OnGossipHello`), and the quest marker comes from `GetDialogStatus`.
The core is not changed.

## Identifiers

Block 94000-94099 of the [WoW-mods registry](https://github.com/Adrestie/WoW-mods/blob/main/ID_RANGES.md):
creature 94000, NPC texts 94000-94001, spawns 9400000-9409999.

## Licence

GPL-2.0-or-later, like AzerothCore.
