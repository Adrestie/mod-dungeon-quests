-- mod-dungeon-quests — the rows of the module's first numbers, taken away.
--
-- Until 2026-10-22 the module used block 94000-94099 of the WoW-mods
-- registry: creature 94000, npc_text 94000-94001, spawns 9400000-9409999.
-- It now uses 94600-94699, and that block goes to mod-guild-level. A world
-- database the old version was installed in keeps those rows; they go here.
--
-- Only the rows that are the module's own go: the creature 94000 whose script
-- is npc_dungeon_quests and its spawns, the two texts by their English
-- wording. Rows another module wrote under those numbers stay, so the file can
-- be replayed at any time.

DELETE `c` FROM `creature` `c`
    JOIN `creature_template` `t` ON `t`.`entry` = `c`.`id`
    WHERE `c`.`guid` BETWEEN 9400000 AND 9409999 AND `c`.`id` = 94000
      AND `t`.`ScriptName` = 'npc_dungeon_quests';

DELETE `l` FROM `creature_template_locale` `l`
    JOIN `creature_template` `t` ON `t`.`entry` = `l`.`entry`
    WHERE `l`.`entry` = 94000 AND `t`.`ScriptName` = 'npc_dungeon_quests';

DELETE `m` FROM `creature_template_model` `m`
    JOIN `creature_template` `t` ON `t`.`entry` = `m`.`CreatureID`
    WHERE `m`.`CreatureID` = 94000 AND `t`.`ScriptName` = 'npc_dungeon_quests';

DELETE FROM `creature_template` WHERE `entry` = 94000 AND `ScriptName` = 'npc_dungeon_quests';

DELETE `l` FROM `npc_text_locale` `l`
    JOIN `npc_text` `n` ON `n`.`ID` = `l`.`ID`
    WHERE (`n`.`ID` = 94000 AND `n`.`text0_0` LIKE 'Every task this place holds passes through my hands%')
       OR (`n`.`ID` = 94001 AND `n`.`text0_0` LIKE 'I have no task for you in this place%');

DELETE FROM `npc_text`
    WHERE (`ID` = 94000 AND `text0_0` LIKE 'Every task this place holds passes through my hands%')
       OR (`ID` = 94001 AND `text0_0` LIKE 'I have no task for you in this place%');
