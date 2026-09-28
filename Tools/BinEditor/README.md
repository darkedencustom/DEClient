# BinEditor

Editor for the gameserver's monster loot files, `DEServer_v664/data/<Monster>.<race>.bin`.
Each monster (or monster tier) has three files, one per killer race: `.slayer.bin`,
`.vampire.bin`, `.ousters.bin`. The editor shows all three as tabs, names every item,
option and monster, and computes the per-kill chance of every item the same way the
server rolls it.

```
BinEditor.exe                        open the window (remembers the last data folder)
BinEditor.exe <folder or .bin file>  open that folder / file
BinEditor.exe dump  FILE...          print a file as a readable tree with names and chances
BinEditor.exe check FOLDER|FILE...   list the problems the server would hit in those files
BinEditor.exe export FILE OUT.json   write a file as JSON (edit by hand, then import)
BinEditor.exe import IN.json OUT.bin write a .bin from JSON
BinEditor.exe roundtrip FOLDER       prove read+write is byte-exact for every .bin there
python bin_editor.py ...             same thing without the exe (Python 3 + tkinter)
```

## Which file is which monster

`MonsterInfo.cpp` (~735) derives the file from the monster row:

* `MonsterClass` 0: the EName with spaces and the trailing `[..]` removed, so
  `Quest Kid [1]` -> `QuestKid.*.bin`. Bosses and named monsters work this way.
* `MonsterClass` N (the `[N]` in the EName): `ClassN.*.bin`, shared by every regular
  monster of that tier and its chiefs. Class1 is level 1-2 ... Class14 is level 132-194.
* Only monsters with `HasTreasure = 1` load a file. A monster whose file is missing is
  skipped at boot and drops nothing.

The left list shows every base name in the data folder with a one-line summary of the
monsters that use it (from `names.json`). The panel above the race tabs lists every
one of those monsters in full (type, names, level, chief / event-spawned / HasTreasure
flags), regular monsters first; drag its bottom edge to make it taller. Tick *show
monsters without a .bin* to see monsters that are flagged for treasure but have no file
yet; *File > New monster loot* creates one.

## Editing

* Select a row; the pane underneath edits it. Enter or *Apply* commits a change,
  Ctrl+Z undoes, Delete removes the row. `+ Entry / + Class / + Type / + Option` add a
  child under the selected row, *Duplicate* copies the selected row after itself.
* Item classes, item types and options are drop-downs with names; you can also type
  a bare number. Equipment shows its equip requirement in brackets, taken from the
  `ReqAbility` column: `[STR 120]`, `[Lv 20]`, `[M, stat sum 30]`, `[Adv 11]` (advance
  level). The pane also shows `ItemLevel`, which is not a player level: it is the
  server's tier index (0-16) used to pick items in a level band for gambles and boxes.
* Ctrl+S saves all three race files of the current monster. The files are written in
  place with no backup copy: `data/` is tracked in git, so `git diff data/` shows what
  changed and `git checkout data/<file>` undoes it.
* *Edit > Copy this race to the other two* is the quick way to make identical loot for
  all races. *Export/Import JSON* lets you edit a file in a text editor.
* **The gameserver reads the files at boot. Restart it after saving.**

## Reading the numbers

The *Per kill / share* column and the pane text follow `Treasure.cpp` exactly:

* **Entry**: every entry rolls independently on each kill. `ItemRatio` 999999 always
  passes. Any other value is stored as 40000 by the server, and the check is
  `rand()%100000 < 40000 * ITEM_PROBE_RATIO / 100`; with the live `ITEM_PROBE_RATIO`
  of 500 (AttrInfo 3) that also always passes. `OptionRatio` is not scaled: 40000 means
  40% of the dropped items get an option, 999999 all of them. The premium ratios apply
  on the premium-player path only.
* **Class**: the winner is picked out of the sum of the class ratios, but the pick must
  satisfy `prev_sum < r < sum`, so a class with ratio R wins `(R-1)/total` of the time
  and a class with ratio 0 or 1 never wins.
* **Type**: picked out of a fixed 100000 against the running sum of type ratios. If the
  ratios sum past 100000, the types after that point can never drop (shown in red). If
  they sum below it, the remainder of class wins drop nothing (shown as a note).
* **Unique items** (`UniqueItemInfo`) need one more roll against `UNIQUE_ITEM_RATIO`
  (AttrInfo 18, currently 0, so they never drop).
* **Option**: share = ratio / sum of the type's option ratios.
* Chiefs re-roll a successful entry up to `CHIEF_ITEM_BONUS_NUM` (AttrInfo 55 = 4) more
  times; the displayed chance is for a regular monster.

Red rows are problems the server will hit: unknown item types (logged as "Invalid Item
Type" at boot, ratio forced to 0), types with no info row (would crash on creation),
unreachable types, zero totals. *Tools > Check every file* lists them for the whole folder.

## names.json

Item, option and monster names come from the live database. Regenerate the file after
adding items, options or monsters, from WSL in the server repo:

```
DE_DB_PASSWORD=... python3 tools/export_treasure_names.py
```

It writes `Tools/BinEditor/names.json` (the D: drive is looked up through `/mnt/d`).
The editor also reads the live `ITEM_PROBE_RATIO` and friends from it; *Tools > Set
ITEM_PROBE_RATIO* overrides the value used for the displayed chances.

## Building the exe

`build.bat` runs a self-test and then PyInstaller (`pip install pyinstaller` once).
`BinEditor.exe` is a single file; keep `names.json` next to it. Exes under `Tools/` are
git-ignored, the sources are not.

## File format

All little-endian int32, no padding, parsed by `Treasure.cpp` `loadFromFile`:

```
[entry count]
  entry:  [ItemRatio][OptionRatio][PremiumItemRatio][PremiumOptionRatio][class count]
    class:  [ItemClass][Ratio][type count]
      type:   [ItemType][Ratio][option count]
        option: [OptionType][Ratio]
```
