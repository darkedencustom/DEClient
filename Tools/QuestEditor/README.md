# QuestEditor

Editor for the quest files of DarkEden, on both sides:

* **Server**: `DEServer_v664/data/SimpleGQuest.xml` and `data/EventGQuest.xml`
  (`GQuestInfoManager::load`). Korean text, UTF-8, parsed with xerces. The server
  decides what a quest asks for, gives, and when it fails.
* **Client**: its own English copies of the same two files, packed inside
  `Data\ui\txt\tutorialetc.rpk` (`C_VS_UI_QUEST_MANAGER::LoadQuestXML`). Every piece of
  quest text the player sees comes from there: title, offer dialog, step lines in the
  journal, NPC dialog at each step, complete / fail messages, reward text.

The two copies must agree on the *structure* (which steps exist, their `index` numbers);
the text is free. The editor opens either file, edits it as a tree, checks it against
what each program accepts, and compares the structure of the two copies.

```
QuestEditor.exe                                  open the window (File > Open, or drop a file on it)
QuestEditor.exe file.xml                         a server quest file
QuestEditor.exe tutorialetc.rpk                  the client pack: pick the entry to edit
QuestEditor.exe tutorialetc.rpk:EventGQuest.xml  a given entry
QuestEditor.exe --check <file>                   print the problems (exit 1 when there are errors)
QuestEditor.exe --compare <fileA> <fileB>        structure differences between two copies (exit 1 when any)
QuestEditor.exe --dump <file> [out.xml] [--client]  write the file the way the editor would save it
QuestEditor.exe --list pack.rpk                  entries of a pack
QuestEditor.exe --unpack pack.rpk folder         extract every entry
QuestEditor.exe --pack out.rpk folder|files...   build a pack
QuestEditor.exe --roundtrip pack.rpk             self-test of the pack writer
```

`<file>` is `x.xml`, `pack.rpk` (first quest list inside) or `pack.rpk:Entry.xml`.
Add `--out report.txt` to write a command's output to a file; otherwise it goes to the
console the command was typed in. The server's files are reachable from Windows as
`\\wsl$\Ubuntu\home\andrew\DEServer_v664\data\SimpleGQuest.xml`.

## The window

* **Left**: the quests (id, title, race, level range, type). The *Find* box filters by any
  text in the quest.
* **Top right**: the selected quest as a tree: `Title`, `Script`, the four sections
  `Happen` / `Complete` / `Fail` / `Reward` and their elements, `CompleteMessage`,
  `FailMessage`, `RewardMessage`, `HappenRegion`.
* **Bottom right**: the selected element: every attribute it has (`a`), every attribute
  the element is known to read but does not have yet (`+`), and its text (`t`). Ids are
  followed by their names in brackets when the name tables are found (see below).

Editing:

* `F2` or double-click a line in the bottom pane to change that attribute or the text
  (text is multi-line; Ctrl+Enter accepts). `F2` on the tree edits the text, or the
  first attribute of an element without text.
* Right-click the tree (or the *Element* menu): add a child or a following element from
  the full vocabulary, grouped as conditions / steps / actions / dialog / containers
  (the new element comes with its usual attributes filled in, and a step gets the next
  free `index` of its section); duplicate, delete, move up / down; add, remove or rename
  attributes; rename the element.
* *Quest* menu: new quest (a skeleton with all four sections), duplicate, delete, change
  id, move in the file. Ctrl+N, Ctrl+D, Ctrl+Delete.
* Ctrl+Z / Ctrl+Y undo and redo everything.
* Ctrl+S saves. A server file is written in place. A pack is rewritten whole with the
  edited entry replaced, then read back through the client's own RAR code to prove it
  (see *Packs* below). *File > Export* writes the XML of a pack entry as a plain file.
* *File > Write client-style XML* chooses what the writer emits: the server copy gets
  real XML (`&`, `<` escaped); the client copy gets raw text, because the client's
  reader does no entity decoding. The box is set from where the file came from.

## Check (Ctrl+K)

Lists what each program would choke on, as errors for the side the file belongs to
and warnings for the other side. Double-click a line to jump to the element.

Server rules (from `GQuestInfo.cpp` and `GQuest*Element.cpp`):

* every quest needs all four sections; a missing one is dereferenced at boot (crash);
* each element's required attributes (`KillMonster num sort`, `SayNPC target`,
  `TouchWayPoint zoneid x y dir`, `GiveItem class type`, `Time limit`, ...): a debug
  server asserts, a release server reads an unset value;
* `KillMonster` needs `<Target type>` children and `sort` should match their number;
* `Race` is `SLAYER` / `VAMPIRE` / `OUSTERS`; `domain` is lowercase `blade` / `sword` /
  `gun` / `heal` / `enchant`; section `type` is `and` / `or` / `sequence`;
* element names the server does not register are skipped (that is how `Script` and
  `Message` inside `Complete` work: they are read by the client only).

Client rules (from `SXml.cpp`, the client's hand-written XML scanner, and
`vs_ui_gamecommon2.cpp`):

* steps in `Complete` / `Fail` need an `index`; the client shows the step text and the
  `Script` / `Message` dialog that carries the same index when the server reports that
  step. Indexes must be unique in a section;
* attribute values cannot contain `"` or `'` (the value ends there) nor `<` `>`;
  text cannot contain `<`; text starting with one of `` </>="' `` loses that character;
* no comments in the file (the editor drops them and says so);
* the client does not decode UTF-8: text is drawn as CP949 (euc-kr). The client's
  `SimpleGQuest.xml` is UTF-8 with a handful of non-ASCII characters (`…`, full-width
  punctuation) that the client cannot show; the check lists them. `EventGQuest.xml` is
  euc-kr. The editor keeps each file's encoding and refuses to save a character the
  code page cannot hold.

Both: numeric attributes, duplicate quest ids, duplicate attributes, `Level min > max`,
quest `type` (only `tutorial`, `charge`, `adventure` are known to the client, and they
need a `HappenRegion` for the map icon).

## Compare

*Tools > Compare structure with the other side* opens the other copy (inside a pack, the
entry with the same name) and lists quests only in one file and quests whose sections
differ in element names, `index` attributes or order. Text is not compared. This is the
check to run after changing steps on one side.

## Names

Ids are shown with names when these files are found upward from the exe's folder or
the open file's folder:

| File | Gives |
|---|---|
| `Tools\BinEditor\names.json` | monster MTypes (`<Target type>`), item classes / types (`GiveItem`), option nicks, quest item types (`GQUEST_ITEM`) |
| `Docs\Zones.md` | zone ids (`zoneid`, `ZoneID`, `warp`) |
| `ReleaseData\Data\info\npc.en.inf` | NPC ids (`target`) |

`names.json` is BinEditor's export from the live database (see `Tools/BinEditor/README.md`).
*Tools > Name tables* says what was found.

## Packs (.rpk)

An `.rpk` is a RAR 2.0 archive, password `cjdgorkr` (`VS_UI_filepath.h`). The client
reads it with the UniquE RAR library bundled in `RarLib/RarFile.cpp`; `urarlib.cpp`
here is a verbatim copy of that reader, so the editor sees exactly what the game sees.

Writing: no RAR compressor is available, so the editor writes a RAR 2.0 archive with
every entry *stored* (uncompressed), *not solid* and *not encrypted*. The client's
reader handles all three the same way (it only decrypts when the entry's password flag
is set, and only decrypts whole 16-byte blocks, which is why stored entries are left
unencrypted rather than padded). `tutorialetc.rpk` grows from 31 KB to about 125 KB.
Every save and every `--pack` reads the result back and compares it byte for byte;
`build.bat` runs the same round trip on `ReleaseData\Data\ui\txt\tutorialetc.rpk`.

`ReleaseData\Data` is tracked in git, so `git diff` shows what changed and
`git checkout` undoes it; no backup copies are written. The client loads the pack from
`darkeden.dpk`, so after saving put it back with the Packing tools
(`Tools/Packing/README.md`: `dpk_replace.py ui\txt\tutorialetc.rpk` or `make_patch.py`).

## Workflow for a new quest

1. Open the server file, *Quest > New quest*, fill in the sections (Korean or any text:
   the server only reads attributes), Ctrl+K, Ctrl+S, restart the gameserver.
2. Open `tutorialetc.rpk` > `SimpleGQuest.xml`, add the same quest with the same id,
   section layout and `index` numbers, with the English text. *Tools > Compare* against
   the server file confirms the two agree. Ctrl+S, then repack `darkeden.dpk`.

*Help > Element reference* (F1) lists every element with its attributes and meaning.
*File > Open last server XML / client pack* reopen what was last used; those paths are
kept in `%LOCALAPPDATA%\DarkEden\QuestEditor.ini`.

## Building

`build.bat` locates Visual Studio 2017 or later and builds `QuestEditor.exe` in this
folder (x86, static CRT, no dependencies), then runs the pack round trip. Sources:
`main.cpp` (window and command line), `QuestXml.*` (tree, parser, writer, encodings),
`QuestSchema.*` (vocabulary, checks, names, compare), `RarPack.*` (pack reader /
writer), `urarlib.*` (the client's RAR reader). Exes under `Tools/` are git-ignored.
