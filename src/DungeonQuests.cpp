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

#include "DungeonQuests.h"

#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DisableMgr.h"
#include "GossipDef.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedGossip.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>

namespace
{
    // Gossip actions of the page buttons: the page to show.
    constexpr uint32 ACTION_PAGE = 1000;

    uint64 MapQuestKey(uint32 mapId, uint32 questId)
    {
        return (uint64(mapId) << 32) | questId;
    }

    // The icons PrepareQuestMenu gives: 4 for a quest in the log (the client
    // sends CMSG_QUESTGIVER_COMPLETE_QUEST), 2 for a quest to take (it sends
    // CMSG_QUESTGIVER_QUERY_QUEST), 0 for one that ends as soon as it is
    // taken.
    uint8 AvailableIcon(Quest const* quest)
    {
        if (!quest->IsAutoComplete())
            return 2;

        if (!quest->IsRepeatable() || quest->IsDaily() || quest->IsWeekly() || quest->IsMonthly())
            return 0;

        return 4;
    }
}

DungeonQuestsMgr* DungeonQuestsMgr::instance()
{
    static DungeonQuestsMgr instance;
    return &instance;
}

void DungeonQuestsMgr::LoadConfig()
{
    _enabled = sConfigMgr->GetOption<bool>("DungeonQuests.Enable", true);
    _respectClass = sConfigMgr->GetOption<bool>("DungeonQuests.RespectClass", true);
    _respectLevel = sConfigMgr->GetOption<bool>("DungeonQuests.RespectLevel", true);
    _pageSize = std::clamp<uint32>(sConfigMgr->GetOption<uint32>("DungeonQuests.PageSize", 30), 1, GOSSIP_MAX_MENU_ITEMS);
}

void DungeonQuestsMgr::LoadQuests()
{
    _questsByMap.clear();
    _mapQuests.clear();

    QueryResult result = WorldDatabase.Query("SELECT `map`, `quest` FROM `mod_dungeon_quests` ORDER BY `map`, `quest`");
    if (!result)
    {
        LOG_WARN("module", "mod-dungeon-quests: `mod_dungeon_quests` is empty or missing, no dungeon quest loaded.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 mapId = fields[0].Get<uint32>();
        uint32 questId = fields[1].Get<uint32>();

        if (!sObjectMgr->GetQuestTemplate(questId))
        {
            LOG_ERROR("sql.sql", "mod-dungeon-quests: `mod_dungeon_quests` lists quest {} for map {}, but the quest does not exist.", questId, mapId);
            continue;
        }

        if (_mapQuests.insert(MapQuestKey(mapId, questId)).second)
        {
            _questsByMap[mapId].push_back(questId);
            ++count;
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> mod-dungeon-quests: {} quests in {} dungeons.", count, _questsByMap.size());
}

std::vector<uint32> const* DungeonQuestsMgr::GetQuests(uint32 mapId) const
{
    auto itr = _questsByMap.find(mapId);
    return itr != _questsByMap.end() ? &itr->second : nullptr;
}

bool DungeonQuestsMgr::HasQuest(uint32 mapId, uint32 questId) const
{
    return _mapQuests.count(MapQuestKey(mapId, questId)) != 0;
}

// What the core checks in Player::CanTakeQuest, without the race (and
// faction), the previous and next quests of the chain, the breadcrumbs, the
// skill, the reputation and the conditions.
bool DungeonQuestsMgr::CanOffer(Player* player, Quest const* quest) const
{
    if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_QUEST, quest->GetQuestId(), player))
        return false;

    if (!player->SatisfyQuestStatus(quest, false) || !player->SatisfyQuestExclusiveGroup(quest, false)
        || !player->SatisfyQuestTimed(quest, false))
        return false;

    if (!player->SatisfyQuestDay(quest, false) || !player->SatisfyQuestWeek(quest, false)
        || !player->SatisfyQuestMonth(quest, false) || !player->SatisfyQuestSeasonal(quest, false))
        return false;

    if (_respectClass && !player->SatisfyQuestClass(quest, false))
        return false;

    if (_respectLevel && !player->SatisfyQuestLevel(quest, false))
        return false;

    return true;
}

void DungeonQuestsMgr::ShowMenu(Player* player, Creature* creature, uint32 page) const
{
    ClearGossipMenuFor(player);

    // Quests in the log first, then the quests to take.
    std::vector<std::pair<uint32, uint8>> items;
    if (std::vector<uint32> const* quests = GetQuests(creature->GetMapId()))
    {
        for (uint32 questId : *quests)
        {
            QuestStatus status = player->GetQuestStatus(questId);
            if (status == QUEST_STATUS_COMPLETE || status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_FAILED)
                items.emplace_back(questId, 4);
        }

        for (uint32 questId : *quests)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (quest && CanOffer(player, quest))
                items.emplace_back(questId, AvailableIcon(quest));
        }
    }

    uint32 pages = (items.size() + _pageSize - 1) / _pageSize;
    if (page >= pages)
        page = pages ? pages - 1 : 0;

    WorldSession* session = player->GetSession();
    if (page > 0)
        if (std::string const* text = session->GetModuleString(DUNGEON_QUESTS_MODULE, DUNGEON_QUESTS_STR_PREVIOUS))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, *text, GOSSIP_SENDER_MAIN, ACTION_PAGE + page - 1);

    if (page + 1 < pages)
        if (std::string const* text = session->GetModuleString(DUNGEON_QUESTS_MODULE, DUNGEON_QUESTS_STR_NEXT))
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, *text, GOSSIP_SENDER_MAIN, ACTION_PAGE + page + 1);

    QuestMenu& questMenu = player->PlayerTalkClass->GetQuestMenu();
    for (size_t i = page * _pageSize; i < items.size() && i < (page + 1) * _pageSize; ++i)
        questMenu.AddMenuItem(items[i].first, items[i].second);

    SendGossipMenuFor(player, items.empty() ? DUNGEON_QUESTS_TEXT_NOTHING : DUNGEON_QUESTS_TEXT_HELLO, creature->GetGUID());
}

uint32 DungeonQuestsMgr::GetDialogStatus(Player* player, Creature* creature) const
{
    uint32 status = DIALOG_STATUS_NONE;
    if (!_enabled)
        return status;

    std::vector<uint32> const* quests = GetQuests(creature->GetMapId());
    if (!quests)
        return status;

    for (uint32 questId : *quests)
    {
        switch (player->GetQuestStatus(questId))
        {
            case QUEST_STATUS_COMPLETE:
                return DIALOG_STATUS_REWARD;
            case QUEST_STATUS_INCOMPLETE:
            case QUEST_STATUS_FAILED:
                if (status == DIALOG_STATUS_NONE)
                    status = DIALOG_STATUS_INCOMPLETE;
                break;
            case QUEST_STATUS_NONE:
                if (status != DIALOG_STATUS_AVAILABLE)
                    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
                        if (CanOffer(player, quest))
                            status = DIALOG_STATUS_AVAILABLE;
                break;
            default:
                break;
        }
    }

    return status;
}

Creature* DungeonQuestsMgr::GetQuestGiver(Player* player, ObjectGuid guid) const
{
    if (!guid.IsCreature() || guid.GetEntry() != DUNGEON_QUESTS_NPC)
        return nullptr;

    return player->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_QUESTGIVER);
}

Quest const* DungeonQuestsMgr::GetDungeonQuest(Creature* creature, uint32 questId) const
{
    if (!HasQuest(creature->GetMapId(), questId))
        return nullptr;

    return sObjectMgr->GetQuestTemplate(questId);
}

bool DungeonQuestsMgr::HandlePacket(Player* player, WorldPacket const& packet) const
{
    uint16 opcode = packet.GetOpcode();
    if (opcode != CMSG_QUESTGIVER_QUERY_QUEST && opcode != CMSG_QUESTGIVER_ACCEPT_QUEST
        && opcode != CMSG_QUESTGIVER_COMPLETE_QUEST && opcode != CMSG_QUESTGIVER_REQUEST_REWARD
        && opcode != CMSG_QUESTGIVER_CHOOSE_REWARD)
        return false;

    if (!_enabled)
        return false;

    WorldPacket data(packet);
    data.rpos(0);

    ObjectGuid guid;
    uint32 questId;
    data >> guid >> questId;

    // Another quest giver: the core's.
    if (!guid.IsCreature() || guid.GetEntry() != DUNGEON_QUESTS_NPC)
        return false;

    Creature* creature = GetQuestGiver(player, guid);
    if (!creature)
        return true;

    Quest const* quest = GetDungeonQuest(creature, questId);
    if (!quest)
    {
        player->PlayerTalkClass->SendCloseGossip();
        return true;
    }

    switch (opcode)
    {
        case CMSG_QUESTGIVER_QUERY_QUEST:
            HandleQueryQuest(player, creature, quest);
            break;
        case CMSG_QUESTGIVER_ACCEPT_QUEST:
            HandleAcceptQuest(player, creature, quest);
            break;
        case CMSG_QUESTGIVER_COMPLETE_QUEST:
            HandleCompleteQuest(player, creature, quest);
            break;
        case CMSG_QUESTGIVER_REQUEST_REWARD:
            HandleRequestReward(player, creature, quest);
            break;
        case CMSG_QUESTGIVER_CHOOSE_REWARD:
        {
            uint32 reward;
            data >> reward;
            HandleChooseReward(player, creature, quest, reward);
            break;
        }
        default:
            break;
    }

    return true;
}

// WorldSession::HandleQuestgiverQueryQuestOpcode, with CanOffer for
// CanTakeQuest.
void DungeonQuestsMgr::HandleQueryQuest(Player* player, Creature* creature, Quest const* quest) const
{
    if (!CanOffer(player, quest))
    {
        player->PlayerTalkClass->SendCloseGossip();
        return;
    }

    if (quest->IsAutoAccept() && player->CanAddQuest(quest, true))
        player->AddQuestAndCheckCompletion(quest, creature);

    if (quest->IsAutoComplete() || !quest->GetQuestMethod())
        player->PlayerTalkClass->SendQuestGiverRequestItems(quest, creature->GetGUID(), player->CanCompleteQuest(quest->GetQuestId()), true);
    else
        player->PlayerTalkClass->SendQuestGiverQuestDetails(quest, creature->GetGUID(), true);
}

// WorldSession::HandleQuestgiverAcceptQuestOpcode, with CanOffer for
// CanTakeQuest. The list comes back, for the next quest to take.
void DungeonQuestsMgr::HandleAcceptQuest(Player* player, Creature* creature, Quest const* quest) const
{
    if (!CanOffer(player, quest) || !player->CanAddQuest(quest, true))
    {
        player->PlayerTalkClass->SendCloseGossip();
        return;
    }

    player->AddQuestAndCheckCompletion(quest, creature);

    if (quest->GetSrcSpell() > 0)
        player->CastSpell(player, quest->GetSrcSpell(), true);

    ShowMenu(player, creature);
}

// WorldSession::HandleQuestgiverCompleteQuest.
void DungeonQuestsMgr::HandleCompleteQuest(Player* player, Creature* creature, Quest const* quest) const
{
    uint32 questId = quest->GetQuestId();
    QuestStatus status = player->GetQuestStatus(questId);
    if (status == QUEST_STATUS_REWARDED)
        return;

    if (status == QUEST_STATUS_NONE && !(quest->IsAutoComplete() && CanOffer(player, quest)))
        return;

    ObjectGuid guid = creature->GetGUID();
    if (status != QUEST_STATUS_COMPLETE)
    {
        if (quest->IsRepeatable())
            player->PlayerTalkClass->SendQuestGiverRequestItems(quest, guid, player->CanCompleteRepeatableQuest(quest), false);
        else
            player->PlayerTalkClass->SendQuestGiverRequestItems(quest, guid, player->CanRewardQuest(quest, false), false);
    }
    else if (quest->GetReqItemsCount())
        player->PlayerTalkClass->SendQuestGiverRequestItems(quest, guid, player->CanRewardQuest(quest, false), false);
    else
        player->PlayerTalkClass->SendQuestGiverOfferReward(quest, guid, true);
}

// WorldSession::HandleQuestgiverRequestRewardOpcode.
void DungeonQuestsMgr::HandleRequestReward(Player* player, Creature* creature, Quest const* quest) const
{
    uint32 questId = quest->GetQuestId();
    QuestStatus status = player->GetQuestStatus(questId);
    if (status == QUEST_STATUS_NONE && !(quest->IsAutoComplete() && CanOffer(player, quest)))
        return;

    if (player->CanCompleteQuest(questId))
        player->CompleteQuest(questId);

    if (player->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
        return;

    player->PlayerTalkClass->SendQuestGiverOfferReward(quest, creature->GetGUID(), true);
}

// WorldSession::HandleQuestgiverChooseRewardOpcode, without the next quest of
// the chain: the list comes back instead.
void DungeonQuestsMgr::HandleChooseReward(Player* player, Creature* creature, Quest const* quest, uint32 reward) const
{
    if (reward >= QUEST_REWARD_CHOICES_COUNT)
        return;

    QuestStatus status = player->GetQuestStatus(quest->GetQuestId());
    if (status == QUEST_STATUS_NONE)
    {
        if (!quest->IsAutoComplete() || !CanOffer(player, quest))
            return;
    }
    else if (status != QUEST_STATUS_COMPLETE && !quest->IsAutoComplete() && quest->GetQuestMethod())
        return;

    if (!player->CanRewardQuest(quest, reward, true))
    {
        player->PlayerTalkClass->SendQuestGiverOfferReward(quest, creature->GetGUID(), true);
        return;
    }

    player->RewardQuest(quest, reward, creature);
    ShowMenu(player, creature);
}
