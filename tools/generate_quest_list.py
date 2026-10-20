#!/usr/bin/env python3
# This file is part of mod-dungeon-quests.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.

"""Writes data/sql/world/dungeon_quests_02_list.sql: the quests of each dungeon.

The dungeons are the normal 5-player dungeons of LFGDungeons.dbc (Classic,
The Burning Crusade, Wrath of the Lich King; not the holiday bosses). A quest
belongs to a dungeon when:

* its QuestSortID is a zone of the dungeon's map (AreaTable.dbc), except the
  zones several dungeons share (Hellfire Citadel, Coilfang Reservoir, Tempest
  Keep, Auchindoun), or
* every one of its objectives is found on that map only, and at least one is
  found there: the creatures to kill (their spawns, through kill credits and
  heroic entries) or gameobjects to use, and the items to bring (the creatures
  and chests that drop them). An objective of unknown origin (a creature that
  scripts summon) is left out of the comparison; an item also sold or found
  in containers counts as found elsewhere.

Left out: the quests without objective (going to see someone), those nobody
gives (except the daily quests, which the core rotates), and the unused ones
(titles such as <UNUSED>, [DEPRECATED], <NYI>).

Usage:
    python generate_quest_list.py --dbc "<server Data>\\dbc" --database acore_world
        [--mysql mysql.exe] [--user root] [--password ...] [--host 127.0.0.1]
"""

import argparse
import collections
import os
import struct
import subprocess
import sys

SHARED_ZONES = {3535, 3905, 3842, 3688}
UNUSED_MARKS = ('UNUSED', 'DEPRECATED', 'NYI', '<TEST', '[PH]', 'ZZOLD', 'TEST ')
QUEST_FLAGS_DAILY = 0x1000
SPECIAL_FLAGS_EXPLORATION_OR_EVENT = 0x2


def read_dbc(path):
    data = open(path, 'rb').read()
    _, count, fields, size, _ = struct.unpack('<4s4I', data[:20])
    strings = data[20 + count * size:]
    rows = [struct.unpack('<%dI' % fields, data[20 + i * size:20 + i * size + fields * 4]) for i in range(count)]

    def text(offset):
        return strings[offset:strings.index(b'\0', offset)].decode('utf-8', 'replace')
    return rows, text


class Database:
    def __init__(self, args):
        self.command = [args.mysql, '-h', args.host, '-u', args.user, '--default-character-set=utf8mb4', '-B', '-N', args.database]
        if args.password:
            self.command.insert(1, '-p' + args.password)

    def query(self, sql):
        out = subprocess.run(self.command + ['-e', sql], capture_output=True, check=True).stdout.decode('utf-8')
        return [line.split('\t') for line in out.splitlines() if line]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--dbc', required=True)
    parser.add_argument('--database', default='acore_world')
    parser.add_argument('--mysql', default='mysql')
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--user', default='root')
    parser.add_argument('--password', default='')
    parser.add_argument('--output', default=os.path.join(os.path.dirname(__file__), '..', 'data', 'sql', 'world', 'dungeon_quests_02_list.sql'))
    args = parser.parse_args()
    db = Database(args)

    # The dungeons: LFGDungeons.dbc, type 1 (dungeon), normal, flags 3 (not a
    # holiday boss).
    rows, text = read_dbc(os.path.join(args.dbc, 'LFGDungeons.dbc'))
    maps = sorted({r[23] for r in rows if r[26] == 1 and r[24] == 0 and r[25] == 3})
    rows, text = read_dbc(os.path.join(args.dbc, 'Map.dbc'))
    map_names = {r[0]: text(r[5]) for r in rows}
    rows, _ = read_dbc(os.path.join(args.dbc, 'AreaTable.dbc'))
    zones = collections.defaultdict(set)
    for r in rows:
        if r[1] in maps and r[2] == 0:
            zones[r[1]].add(r[0])

    creature_maps = collections.defaultdict(set)
    for entry, map_id in db.query('SELECT id, map FROM creature'):
        creature_maps[int(entry)].add(int(map_id))
    gameobject_maps = collections.defaultdict(set)
    for entry, map_id in db.query('SELECT id, map FROM gameobject'):
        gameobject_maps[int(entry)].add(int(map_id))

    # A credited entry -> the entries whose spawns give that credit.
    spawned_as = collections.defaultdict(set)
    loot_of = collections.defaultdict(set)
    for entry, credit1, credit2, heroic1, heroic2, heroic3, loot in db.query(
            'SELECT entry, KillCredit1, KillCredit2, difficulty_entry_1, difficulty_entry_2, difficulty_entry_3, lootid FROM creature_template'):
        entry = int(entry)
        spawned_as[entry].add(entry)
        for other in (credit1, credit2, heroic1, heroic2, heroic3):
            if int(other):
                spawned_as[int(other)].add(entry)
        if int(loot):
            loot_of[int(loot)].add(entry)

    def npc_maps(entry):
        found = set()
        for spawned in spawned_as.get(entry, {entry}):
            found |= creature_maps.get(spawned, set())
        return found

    reference_items = collections.defaultdict(set)
    for entry, item, reference in db.query('SELECT Entry, Item, Reference FROM reference_loot_template'):
        reference_items[int(entry)].add(int(item))

    def loot_items(item, reference):
        return reference_items.get(int(reference), set()) if int(reference) else {int(item)}

    UNKNOWN, ELSEWHERE = -1, -2
    item_maps = collections.defaultdict(set)
    for loot, item, reference in db.query('SELECT Entry, Item, Reference FROM creature_loot_template'):
        for it in loot_items(item, reference):
            for creature in loot_of.get(int(loot), ()):
                item_maps[it] |= npc_maps(creature) or {UNKNOWN}
    chests = collections.defaultdict(set)
    for entry, loot in db.query('SELECT entry, Data1 FROM gameobject_template WHERE type IN (3, 25)'):
        chests[int(loot)].add(int(entry))
    for loot, item, reference in db.query('SELECT Entry, Item, Reference FROM gameobject_loot_template'):
        for it in loot_items(item, reference):
            for chest in chests.get(int(loot), ()):
                item_maps[it] |= gameobject_maps.get(chest, set()) or {UNKNOWN}
    for (item,) in db.query('SELECT item FROM npc_vendor'):
        item_maps[int(item)].add(ELSEWHERE)
    for _, item, reference in db.query('SELECT Entry, Item, Reference FROM item_loot_template'):
        for it in loot_items(item, reference):
            item_maps[it].add(ELSEWHERE)

    given = {int(r[0]) for r in db.query(
        'SELECT quest FROM creature_queststarter UNION SELECT quest FROM gameobject_queststarter '
        'UNION SELECT startquest FROM item_template WHERE startquest <> 0')}

    quests = collections.defaultdict(list)
    for r in db.query(
            'SELECT q.ID, q.QuestSortID, q.LogTitle, q.Flags, IFNULL(a.SpecialFlags, 0), '
            'q.RequiredNpcOrGo1, q.RequiredNpcOrGo2, q.RequiredNpcOrGo3, q.RequiredNpcOrGo4, '
            'q.RequiredItemId1, q.RequiredItemId2, q.RequiredItemId3, q.RequiredItemId4, q.RequiredItemId5, q.RequiredItemId6, '
            'q.StartItem, q.ItemDrop1, q.ItemDrop2, q.ItemDrop3, q.ItemDrop4 '
            'FROM quest_template q LEFT JOIN quest_template_addon a ON a.ID = q.ID ORDER BY q.ID'):
        quest, sort, title, flags, special = int(r[0]), int(r[1]), r[2], int(r[3]), int(r[4])
        if title.startswith('<') or any(mark in title.upper() for mark in UNUSED_MARKS):
            continue
        if quest not in given and not flags & QUEST_FLAGS_DAILY:
            continue
        targets = [int(x) for x in r[5:9] if int(x)]
        items = [int(x) for x in r[9:15] if int(x)]
        provided = {int(x) for x in r[15:20] if int(x)}
        if not targets and not items and not special & SPECIAL_FLAGS_EXPLORATION_OR_EVENT:
            continue

        objectives = [npc_maps(t) if t > 0 else gameobject_maps.get(-t, set()) for t in targets]
        objectives += [item_maps.get(it, set()) for it in items if it not in provided]
        objectives = [o - {UNKNOWN} for o in objectives]

        for map_id in maps:
            if sort > 0 and sort in zones[map_id] and sort not in SHARED_ZONES:
                quests[map_id].append((quest, title))
            elif objectives and all(o <= {map_id} for o in objectives) and any(o == {map_id} for o in objectives):
                quests[map_id].append((quest, title))

    with open(args.output, 'w', encoding='utf-8', newline='\n') as out:
        out.write('-- mod-dungeon-quests — the quests of each dungeon, by map.\n')
        out.write('--\n')
        out.write('-- Written by tools/generate_quest_list.py from the world database and the\n')
        out.write('-- DBC files; the rules are at the top of that script. The table may be\n')
        out.write('-- edited by hand: `.reload config` reads it again.\n')
        out.write('--\n')
        out.write('-- Idempotent: the table is emptied before being filled again.\n\n')
        out.write('CREATE TABLE IF NOT EXISTS `mod_dungeon_quests` (\n')
        out.write('    `map` SMALLINT UNSIGNED NOT NULL,\n')
        out.write('    `quest` INT UNSIGNED NOT NULL,\n')
        out.write("    `comment` VARCHAR(255) NOT NULL DEFAULT '',\n")
        out.write('    PRIMARY KEY (`map`, `quest`)\n')
        out.write(") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-dungeon-quests: the quests of each dungeon';\n\n")
        out.write('DELETE FROM `mod_dungeon_quests`;\n')
        out.write('INSERT INTO `mod_dungeon_quests` (`map`, `quest`, `comment`) VALUES\n')
        lines = []
        for map_id in maps:
            for quest, title in quests[map_id]:
                comment = (map_names[map_id] + ': ' + title).replace('\\', '\\\\').replace("'", "''")
                lines.append("(%d, %d, '%s')" % (map_id, quest, comment))
        out.write(',\n'.join(lines) + ';\n')

    print('%d quests in %d dungeons -> %s' % (sum(len(q) for q in quests.values()), len(quests), os.path.normpath(args.output)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
