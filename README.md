# mod-dungeon-quests

Module AzerothCore pour World of Warcraft 3.3.5a (Wrath of the Lich King).

Un PNJ neutre, l'**Émissaire des quêtes**, se tient à l'entrée et à la fin de
chaque donjon à 5 joueurs (Classique, The Burning Crusade, Wrath of the Lich
King). Il donne et valide les quêtes qui s'accomplissent dans ce donjon, **quels
que soient la race, la faction et les suites de quêtes** : un joueur de la
Horde peut prendre les quêtes de l'Alliance des Mortemines, et inversement,
sans avoir fait les quêtes qui y mènent.

## En jeu

- Le PNJ montre la vraie fenêtre de quêtes : la liste, le texte de chaque
  quête, ses récompenses (au choix quand il y en a plusieurs).
- Les quêtes du journal viennent en tête de liste (point d'interrogation), puis
  celles à prendre (point d'exclamation). Une quête prise, la liste revient
  pour prendre la suivante.
- Une quête terminée se rend au même PNJ, à l'entrée ou à la fin du donjon.
- Une liste trop longue pour la fenêtre (32 quêtes au plus) se découpe en
  pages, avec des boutons « Autres quêtes... » et « Quêtes précédentes... ».
- Le PNJ porte l'icône de quête habituelle sur la minicarte et au-dessus de
  sa tête : point d'interrogation jaune quand une quête est à rendre, point
  d'exclamation quand une quête est à prendre.

Ce qui est ignoré : la race (et donc la faction), les quêtes précédentes et
suivantes de la chaîne, les quêtes d'introduction, la compétence, la
réputation et les conditions de la base. Ce qui reste vérifié : la quête n'est
pas déjà dans le journal ni déjà rendue, les quêtes journalières et
hebdomadaires une fois par jour ou par semaine, les quêtes saisonnières pendant
leur fête, les quêtes exclusives entre elles (la version Alliance et la version
Horde d'une même quête ne se font pas toutes les deux quand la base les lie),
la place dans le journal et, par défaut, la classe et le niveau minimum (voir
la configuration).

Une quête rendue à l'Émissaire ne propose pas la quête suivante de sa chaîne :
la suite normale dans le monde peut ne plus être faisable. C'est voulu.

## Les quêtes de chaque donjon

La table `mod_dungeon_quests` (base world : `map`, `quest`, `comment`) liste
les quêtes de chaque donjon ; la carte où se tient le PNJ choisit sa liste, si
bien qu'une seule créature (94000) sert partout. Elle est écrite par
`tools/generate_quest_list.py` à partir de la base world et des fichiers DBC
du serveur. Une quête appartient à un donjon quand :

- sa zone (`QuestSortID`) est celle du donjon, sauf les zones que plusieurs
  donjons partagent (Citadelle des Flammes infernales, Réservoir de Glissecroc,
  Donjon de la Tempête, Auchindoun) ;
- ou tous ses objectifs ne se trouvent que sur la carte du donjon : les
  créatures à tuer (leurs apparitions, crédits de victime et versions
  héroïques comprises), les objets à utiliser, les objets à rapporter (les
  créatures et coffres qui les donnent).

Sont écartées les quêtes sans objectif (aller parler à quelqu'un), celles que
personne ne donne (sauf les journalières, que le cœur fait tourner) et les
quêtes inutilisées. Le résultat actuel : 339 quêtes dans 51 donjons, quêtes
journalières et héroïques comprises quand elles se font dans le donjon.

Pour ajuster la liste à la main, modifiez la table puis tapez
`.reload config` : le module la relit. Ces changements sont perdus quand le
module est réinstallé ou mis à jour ; pour les garder, modifiez
`data/sql/world/dungeon_quests_02_list.sql` (ou les règles du script) avant
d'installer. Pour regénérer la liste :

```
python tools/generate_quest_list.py --dbc "<Data du serveur>\dbc" --database acore_world --mysql "<chemin de mysql.exe>" --password <mot de passe>
```

## Les emplacements

117 PNJ, dans `data/sql/world/dungeon_quests_03_spawns.sql` (guids
9400000-9400116) :

- à l'entrée, près du point d'arrivée des joueurs (une par aile pour le
  Monastère écarlate, Hache-Tripes, Maraudon, Stratholme ; les deux portes
  d'Uldaman) ;
- à la fin, à côté du dernier boss du donjon (ou de chaque aile), ou de la
  créature ou du coffre qui clôt le donjon quand le boss est invoqué par son
  script. Pas de PNJ de fin dans l'Oculus (le dernier boss se combat à dos de
  drake) ni au Fort pourpre (une seule salle, le PNJ d'entrée suffit).

Le PNJ se pose sur le sol à son apparition, si son emplacement est un peu
au-dessus ou au-dessous. Beaucoup des emplacements d'entrée reprennent ceux du
module Dungeon-Quest-Pet (son « Bryten Helper »), dont ce module s'inspire.

## Installation

Avec [WoW-mods-installer](https://github.com/Adrestie/WoW-mods-installer), ou à
la main :

1. Copiez ce dossier dans `modules/` des sources d'AzerothCore.
2. Relancez CMake (sans quoi le nouveau module est ignoré), puis compilez.
3. Copiez `conf/mod-dungeon-quests.conf.dist` dans le dossier
   `configs/modules` du serveur, sous le nom `mod-dungeon-quests.conf`.
4. Au démarrage, le worldserver applique les fichiers de `data/sql/world` à la
   base world (si `Updates.EnableDatabases` le permet ; sinon appliquez-les
   dans l'ordre).

Aucune table de la base characters : le module ne garde rien des joueurs.

## Configuration

| Réglage | Défaut | Effet |
|---|---|---|
| `DungeonQuests.Enable` | 1 | 0 : les PNJ n'ont rien à proposer. |
| `DungeonQuests.RespectClass` | 1 | 0 : les quêtes de classe sont proposées à toutes les classes. |
| `DungeonQuests.RespectLevel` | 1 | 0 : les quêtes sont proposées avant leur niveau minimum. |
| `DungeonQuests.PageSize` | 30 | Nombre de quêtes par page de la liste (32 au plus). |

## Fonctionnement

La créature n'a aucune ligne dans `creature_queststarter` ni
`creature_questender`. Les paquets de quête que le client lui envoie
(`CMSG_QUESTGIVER_QUERY_QUEST`, `ACCEPT_QUEST`, `COMPLETE_QUEST`,
`REQUEST_REWARD`, `CHOOSE_REWARD`) sont traités par le module avant les
gestionnaires du cœur (crochet `CanPacketReceive`, dans le même fil que ces
gestionnaires), avec ses propres vérifications à la place de
`Player::CanTakeQuest`. La liste est un menu de dialogue
(`OnGossipHello`), l'icône vient de `GetDialogStatus`. Aucun changement du
cœur.

## Identifiants

Tranche 94 du [registre WoW-mods](https://github.com/Adrestie/WoW-mods/blob/main/ID_RANGES.md) :
créature 94000, textes de PNJ 94000-94001, apparitions 9400000-9499999.

## Licence

GPL-2.0-or-later, comme AzerothCore.
