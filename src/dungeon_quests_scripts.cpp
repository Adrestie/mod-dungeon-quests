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
 * mod-dungeon-quests — the hooks. Each one hands over to DungeonQuestsMgr.
 */

#include "DungeonQuests.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

#include <cmath>

class DungeonQuestsWorldScript : public WorldScript
{
public:
    DungeonQuestsWorldScript() : WorldScript("DungeonQuestsWorldScript",
        { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP }) { }

    // `.reload config` also reads the quest list again.
    void OnAfterConfigLoad(bool reload) override
    {
        sDungeonQuestsMgr->LoadConfig();
        if (reload)
            sDungeonQuestsMgr->LoadQuests();
    }

    void OnStartup() override
    {
        sDungeonQuestsMgr->LoadQuests();
    }
};

// The quest packets sent to the quest giver, answered before the core's
// handlers (and in the same thread: the session's update).
class DungeonQuestsServerScript : public ServerScript
{
public:
    DungeonQuestsServerScript() : ServerScript("DungeonQuestsServerScript", { SERVERHOOK_CAN_PACKET_RECEIVE }) { }

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        Player* player = session ? session->GetPlayer() : nullptr;
        if (!player || !player->IsInWorld())
            return true;

        return !sDungeonQuestsMgr->HandlePacket(player, packet);
    }
};

class npc_dungeon_quests : public CreatureScript
{
public:
    npc_dungeon_quests() : CreatureScript("npc_dungeon_quests") { }

    // The spots of the spawns are taken beside an arrival point or a boss: the
    // quest giver settles on the floor below it once, when it first updates.
    struct npc_dungeon_questsAI : public ScriptedAI
    {
        npc_dungeon_questsAI(Creature* creature) : ScriptedAI(creature) { }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (_settled)
                return;

            _settled = true;
            float x = me->GetPositionX();
            float y = me->GetPositionY();
            float z = me->GetPositionZ();
            float o = me->GetOrientation();
            float ground = me->GetMapHeight(x, y, z + 2.0f);
            if (ground <= INVALID_HEIGHT || std::fabs(ground - z) < 0.2f || std::fabs(ground - z) > 10.0f)
                return;

            me->NearTeleportTo(x, y, ground, o);
            me->SetHomePosition(x, y, ground, o);
        }

    private:
        bool _settled = false;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_dungeon_questsAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!sDungeonQuestsMgr->IsEnabled())
            return true;

        sDungeonQuestsMgr->ShowMenu(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (!sDungeonQuestsMgr->IsEnabled())
        {
            CloseGossipMenuFor(player);
            return true;
        }

        // The page buttons: 1000 + the page.
        sDungeonQuestsMgr->ShowMenu(player, creature, action >= 1000 ? action - 1000 : 0);
        return true;
    }

    uint32 GetDialogStatus(Player* player, Creature* creature) override
    {
        return sDungeonQuestsMgr->GetDialogStatus(player, creature);
    }
};

void AddSC_dungeon_quests_scripts()
{
    new DungeonQuestsWorldScript();
    new DungeonQuestsServerScript();
    new npc_dungeon_quests();
}
