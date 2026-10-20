/*
 * This file is part of mod-dungeon-quests.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * mod-dungeon-quests — a neutral quest giver at the entrance and at the end of
 * every dungeon. It gives and takes back the quests done in its dungeon,
 * whatever the race, the faction or the chain of the quest.
 *
 * The quests of each dungeon are listed in `mod_dungeon_quests` (world
 * database): the map the creature stands on picks its list, so one creature
 * entry serves every dungeon. The creature has no row in
 * `creature_queststarter` / `creature_questender`: the quest packets the
 * client sends to it are answered here, before the core handlers, which would
 * check the race, the faction and the previous quests.
 */

#ifndef MOD_DUNGEON_QUESTS_H
#define MOD_DUNGEON_QUESTS_H

#include "Define.h"
#include "ObjectGuid.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

class Creature;
class Player;
class Quest;
class WorldPacket;

constexpr char const* DUNGEON_QUESTS_MODULE = "mod-dungeon-quests";

// Tranche 94 of the WoW-mods registry.
enum DungeonQuestsIds : uint32
{
    DUNGEON_QUESTS_NPC          = 94000,  // the quest giver, every dungeon
    DUNGEON_QUESTS_TEXT_HELLO   = 94000,  // npc_text: quests to give or take back
    DUNGEON_QUESTS_TEXT_NOTHING = 94001,  // npc_text: nothing for this player
};

// module_string ids
enum DungeonQuestsStrings : uint32
{
    DUNGEON_QUESTS_STR_NEXT     = 1,
    DUNGEON_QUESTS_STR_PREVIOUS = 2,
};

class DungeonQuestsMgr
{
public:
    static DungeonQuestsMgr* instance();

    void LoadConfig();
    void LoadQuests();

    bool IsEnabled() const { return _enabled; }

    // The quests of a map, in the order of the table.
    std::vector<uint32> const* GetQuests(uint32 mapId) const;
    bool HasQuest(uint32 mapId, uint32 questId) const;

    // A quest of the creature's dungeon this player may take from it now.
    bool CanOffer(Player* player, Quest const* quest) const;

    // The quest list, as gossip, from the given page.
    void ShowMenu(Player* player, Creature* creature, uint32 page = 0) const;
    uint32 GetDialogStatus(Player* player, Creature* creature) const;

    // The quest packets the client sends to the creature. True: answered here,
    // the core handler is skipped.
    bool HandlePacket(Player* player, WorldPacket const& packet) const;

private:
    Creature* GetQuestGiver(Player* player, ObjectGuid guid) const;
    Quest const* GetDungeonQuest(Creature* creature, uint32 questId) const;

    void HandleQueryQuest(Player* player, Creature* creature, Quest const* quest) const;
    void HandleAcceptQuest(Player* player, Creature* creature, Quest const* quest) const;
    void HandleCompleteQuest(Player* player, Creature* creature, Quest const* quest) const;
    void HandleRequestReward(Player* player, Creature* creature, Quest const* quest) const;
    void HandleChooseReward(Player* player, Creature* creature, Quest const* quest, uint32 reward) const;

    bool _enabled = true;
    bool _respectClass = true;
    bool _respectLevel = true;
    uint32 _pageSize = 30;

    std::unordered_map<uint32, std::vector<uint32>> _questsByMap;
    std::unordered_set<uint64> _mapQuests;  // map << 32 | quest
};

#define sDungeonQuestsMgr DungeonQuestsMgr::instance()

#endif
