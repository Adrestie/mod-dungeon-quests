-- mod-dungeon-quests — the quest giver of the dungeons and its texts.
--
--   creature 94000   the quest giver, the same in every dungeon: the map it
--                    stands on picks its quests (`mod_dungeon_quests`)
--   npc_text 94000   its greeting when it has quests for the player
--   npc_text 94001   its greeting when it has none
--
-- The look of Archmage Timear (31439), the Kirin Tor mage who sends players
-- into the dungeons of Northrend; neutral (faction 35), gossip and quest giver.
-- No `creature_queststarter` / `creature_questender` row: the module answers
-- the quest packets itself.
--
-- Tranche 94 of the WoW-mods registry. Idempotent: every row is deleted before
-- being written again.

DELETE FROM `creature_template` WHERE `entry` = 94000;
INSERT INTO `creature_template` (`entry`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`,
    `faction`, `npcflag`, `speed_walk`, `speed_run`, `rank`, `unit_class`, `unit_flags`, `unit_flags2`, `type`,
    `type_flags`, `RegenHealth`, `flags_extra`, `AIName`, `ScriptName`)
VALUES (94000, 'Quest Emissary', 'Dungeon Quests', '', 0, 80, 80, 2,
    35, 3, 1, 1.14286, 0, 8, 768, 2048, 7,
    0, 1, 2, '', 'npc_dungeon_quests');

DELETE FROM `creature_template_model` WHERE `CreatureID` = 94000;
INSERT INTO `creature_template_model` (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`) VALUES
(94000, 0, 27592, 1, 1, 0);

DELETE FROM `creature_template_locale` WHERE `entry` = 94000;
INSERT INTO `creature_template_locale` (`entry`, `locale`, `Name`, `Title`, `VerifiedBuild`) VALUES
(94000, 'frFR', 'Émissaire des quêtes', 'Quêtes de donjon', 0),
(94000, 'deDE', 'Questgesandter', 'Dungeonquests', 0),
(94000, 'esES', 'Emisario de misiones', 'Misiones de mazmorra', 0),
(94000, 'esMX', 'Emisario de misiones', 'Misiones de mazmorra', 0),
(94000, 'ruRU', 'Посланник заданий', 'Задания подземелья', 0);

DELETE FROM `npc_text` WHERE `ID` IN (94000, 94001);
INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `Probability0`) VALUES
(94000, 'Every task this place holds passes through my hands, whoever sent you and whatever banner you march under. Take what you will, and come back to me when it is done.', '', 1),
(94001, 'I have no task for you in this place. Perhaps another dungeon holds work for you.', '', 1);

DELETE FROM `npc_text_locale` WHERE `ID` IN (94000, 94001);
INSERT INTO `npc_text_locale` (`ID`, `Locale`, `Text0_0`, `Text0_1`) VALUES
(94000, 'frFR', 'Toutes les tâches de ce lieu passent entre mes mains, qui que soit celui qui vous envoie et quelle que soit votre bannière. Prenez ce que vous voulez, et revenez me voir quand ce sera fait.', ''),
(94000, 'deDE', 'Jede Aufgabe dieses Ortes geht durch meine Hände, wer auch immer Euch schickt und unter welchem Banner Ihr auch marschiert. Nehmt, was Ihr wollt, und kehrt zu mir zurück, wenn es vollbracht ist.', ''),
(94000, 'esES', 'Todas las tareas de este lugar pasan por mis manos, os envíe quien os envíe y bajo el estandarte que sea. Tomad lo que queráis y volved a verme cuando esté hecho.', ''),
(94000, 'esMX', 'Todas las tareas de este lugar pasan por mis manos, te envíe quien te envíe y bajo el estandarte que sea. Toma lo que quieras y vuelve a verme cuando esté hecho.', ''),
(94000, 'ruRU', 'Все задания этого места проходят через мои руки, кто бы вас ни послал и под каким бы знаменем вы ни шли. Берите что пожелаете и возвращайтесь ко мне, когда дело будет сделано.', ''),
(94001, 'frFR', 'Je n''ai aucune tâche pour vous en ce lieu. Un autre donjon aura peut-être du travail pour vous.', ''),
(94001, 'deDE', 'Ich habe an diesem Ort keine Aufgabe für Euch. Vielleicht hält ein anderer Dungeon Arbeit für Euch bereit.', ''),
(94001, 'esES', 'No tengo ninguna tarea para vos en este lugar. Quizá otra mazmorra tenga trabajo para vos.', ''),
(94001, 'esMX', 'No tengo ninguna tarea para ti en este lugar. Quizá otra mazmorra tenga trabajo para ti.', ''),
(94001, 'ruRU', 'В этом месте у меня нет для вас заданий. Возможно, в другом подземелье для вас найдется работа.', '');

-- The page buttons of a long list.
DELETE FROM `module_string` WHERE `module` = 'mod-dungeon-quests';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-dungeon-quests', 1, 'More quests...'),
('mod-dungeon-quests', 2, 'Previous quests...');

DELETE FROM `module_string_locale` WHERE `module` = 'mod-dungeon-quests';
INSERT INTO `module_string_locale` (`module`, `id`, `locale`, `string`) VALUES
('mod-dungeon-quests', 1, 'frFR', 'Autres quêtes...'),
('mod-dungeon-quests', 2, 'frFR', 'Quêtes précédentes...'),
('mod-dungeon-quests', 1, 'deDE', 'Weitere Quests...'),
('mod-dungeon-quests', 2, 'deDE', 'Vorherige Quests...'),
('mod-dungeon-quests', 1, 'esES', 'Más misiones...'),
('mod-dungeon-quests', 2, 'esES', 'Misiones anteriores...'),
('mod-dungeon-quests', 1, 'esMX', 'Más misiones...'),
('mod-dungeon-quests', 2, 'esMX', 'Misiones anteriores...'),
('mod-dungeon-quests', 1, 'ruRU', 'Другие задания...'),
('mod-dungeon-quests', 2, 'ruRU', 'Предыдущие задания...');
