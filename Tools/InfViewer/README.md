# InfViewer

Read-only Windows viewer for the DarkEden client's binary `Data\Info\*.inf`
tables. It decodes each file with the same field layout the client's own
`LoadFromFile` code uses, so you can read exactly which skill (ACTIONINFO)
plays which effect (EffectSpriteType / FrameID), which sound, which frames,
and so on, without touching sprites.

```
InfViewer.exe                       open the window, then File > Open or drop a file on it
InfViewer.exe Data\Info\Action.inf  open a file straight away
InfViewer.exe --dump in.inf [out.txt] [--schema id]
                                    no UI: write the decoded text and exit
                                    (exit 0 = whole file decoded, 2 = incomplete, 1 = error)
```

## Using it

* **Left pane** lists the records: `#` is the table index or map id (for
  Action.inf and Skill.inf that is the ACTIONINFO id, for EffectSpriteType.inf
  the EffectSpriteType id, for Item.inf `class:type`), plus a summary and the
  byte offset of the record in the file.
* **Right pane** shows every field of the selected record. `Ctrl+Shift+C`
  copies it.
* **Find** (`Ctrl+F`, `F3` for the next hit) searches keys, summaries, field
  names and values of every record, so typing an effect id or a skill name
  jumps to it.
* **Schema** is picked from the file name (`skill.en.inf` uses the `skill`
  layout). Override it from the drop-down if a file is named oddly. Two
  fallbacks exist for unknown files: a length-prefixed string scan and a hex
  dump.
* **File > Export** writes the whole decoded table as UTF-8 text so you can
  diff it or grep it with other tools. `View > File info` (`Ctrl+I`) shows the
  header fields, the source loader the schema was taken from, and whether
  every byte of the file was accounted for.

Korean strings are CP949 and are converted for display.

## Following an effect

1. Open `Action.inf`, find the skill by name (for example `SKILL_BLOODY_WAVE`).
   `ActionEffectSpriteType` is the body effect; each `Node[n]` row is one step
   with its own `EffectSpriteType`, `EffectGeneratorID` and `SoundID`.
2. Open `EffectSpriteType.inf` and jump to that id: `FrameID` is the frame in
   `Effect.spk`, `PairFrameIDList` the frames drawn alongside, and the colour
   groups swap in other EffectSpriteTypes per colour.
3. `EffectStatus.inf` maps status effects back to an EffectSpriteType and the
   ActionInfo that caused them; `Skill.inf` / `SkillInfo.inf` hold the tree
   entry and the server-side numbers for the same ACTIONINFO id.

## Editing

You never need a byte offset: the decoder remembers where every field, string
list and record sits, and each edit rewrites that span (length prefixes and
counts included) and shifts the rest of the file.

* **Change a value**: put the caret on a field line in the right pane and
  press `F2` (or double-click the line). Lines marked `#` are integers (decimal
  or `0x..`), lines marked `s` are strings (multi-line allowed, `Ctrl+Enter`
  to accept). Unmarked lines are derived values or structural counts and are
  read-only; edit their parts instead.
* **Add a record**: `Edit > Add record` (`Ctrl+N`) clones the selected record
  onto the end of the table. Keyed tables (NPCScript, NPC, ShopTemplate, Zone,
  SkillInfo, QuestInfo, ...) get the next free id automatically; index tables
  (Action, Skill, Creature, Item options, ...) get the next index. Then edit
  its fields.
* **Script lines**: `Edit > Lists` appends a line to, or removes the last line
  of, any string list in the selected record (NPCScript `Subject` / `Content`,
  Zone `MapShape`).
* **Delete a record**: `Edit > Delete` (`Del` in the list). For index tables
  this renumbers everything after it, and the viewer says so before doing it.
* **Save**: `Ctrl+S` writes the file in place. `Ctrl+Shift+S` saves a copy
  elsewhere.

Every edit is re-decoded before it is accepted, and rejected if the file would
no longer be fully accounted for, so a broken table cannot be written. Edit the
variant the client actually loads: with English data that is `foo.en.inf`,
otherwise `foo.inf`. `Item.inf` is a nested class/type table, so fields can be
edited but records cannot be added there; the sprite tables (`Creature.inf`
etc.) can grow, but a new creature also needs sprite data the viewer does not
manage.

Tab moves between the find box, the record list and the detail pane.

## Building

`build.bat` locates Visual Studio 2017-2022 and builds `InfViewer.exe` in this
folder (x86, static CRT, no dependencies). `InfViewer.sln` opens the same two
sources in the IDE. The verification loop used while writing this was:

```
for %f in (..\..\Release\Data\info\*.inf) do InfViewer.exe --dump "%f" "%TEMP%\%~nxf.txt"
```

All 78 `.inf` files shipped in `Release` decode to exactly their file size.

## Schemas

| File(s) | Source of the layout |
|---|---|
| Action.inf | `MActionInfoTable::LoadFromFile`, `MActionInfo::LoadFromFile`, `ACTION_INFO_NODE` |
| Skill.inf | `SKILLINFO_NODE::LoadFromFile` |
| SkillInfo.inf | `SKILLINFO_NODE::LoadFromFileServerSkillInfo` |
| SkillManager.inf | `CTypeTable<MSkillDomain>` (no longer loaded by the client) |
| DomainExp.inf | `MSkillManager::LoadFromFileServerDomainInfo` |
| *Exp.inf, StrBalanceInfo.inf | `ExperienceTable::LoadFromFile*` |
| EffectSpriteType.inf | `EFFECTSPRITETYPETABLE_INFO::LoadFromFile` |
| ActionEffectSpriteType.inf | `MActionEffectSpriteTypeTable` |
| EffectStatus.inf | `EFFECTSTATUS_NODE::LoadFromFile` |
| CreatureSprite.inf, AddonSprite.inf, CreatureActionSprite.inf | `CREATURESPRITETABLE_INFO::LoadFromFile` |
| CreatureSpriteMapper.inf | `CreatureSpriteTypeMapper::LoadFromFile` |
| Creature.inf | `CREATURETABLE_INFO::LoadFromFile` (action arrays sized by `GetActionMax`) |
| Item.inf | `ITEMCLASS_TABLE` / `ITEMTYPE_TABLE` / `ITEMTABLE_INFO::LoadFromFile` (auto-detects the `__ONIBLA_ITEM` field) |
| ItemOption.inf | `ITEMOPTION_TABLE::LoadFromFile` |
| NPC.inf, NPCScript.inf, ShopTemplate.inf | `CTypeMap` tables |
| Zone.inf | `CZoneTable::LoadFromFile`, `WORLDMAP_INFO` |
| String.inf, LevelName.inf, SHelpString.inf, Help.inf | `MStringArray` |
| MonsterName.inf, Nickname.inf | `MonsterNameTable`, `LoadFromFile_NickNameString` |
| Sound.inf, Music.inf, FameLimit.inf, RankBonus.inf, GuildMapper.inf, UsingColorSet.inf, InteractionObject.inf | the matching `*_INFO::LoadFromFile` |
| QuestInfo.inf, Player.inf, Chat.inf, Gps.inf, Horn.inf, EventQuest.inf, PowerjjangItem.inf, AppendPatch.inf, SHelp.inf, ClientConfig.inf | the matching loader in `UI/` |
| Waypoint.inf | inferred (count + int triples); nothing in the client reads it any more |
| Version.inf, GuildVersion.inf, IpAddr.inf | raw integers |
| FileDef.inf, GameClient.inf, Interface.inf, ... | plain text |
