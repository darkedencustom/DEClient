// QuestSchema.h - what a quest file may contain, how to check one, and the
// names shown next to ids.
//
// The element vocabulary comes from the game server: GQuestElementFactory
// registers one prototype per element name (GQuest*Element.h), and each
// element's makeElement reads the attributes listed here (an Assert on a
// missing one aborts a debug server; a release server reads garbage).
// Names the server does not know (Script, Message, ...) are skipped by the
// server and read only by the client, which shows their text as NPC dialog
// and finds them by their "index" attribute.
#pragma once
#include "QuestXml.h"
#include <map>
#include <string>
#include <vector>

enum ElemGroup
{
    EG_QUEST = 0,     // direct children of <Quest> that carry text or the region
    EG_SECTION,       // Happen / Complete / Fail / Reward
    EG_CONDITION,     // checked instantly (Happen) or as a step (Complete / Fail)
    EG_STEP,          // things the player has to do; have "index" and step text
    EG_ACTION,        // done to the player (Reward, or inline in Complete)
    EG_TEXT,          // client-only dialog
    EG_CONTAINER,     // NOT / OR / Random / Target
    EG_GROUPS
};

enum AttrKind
{
    AK_TEXT = 0, AK_INT, AK_BOOL, AK_RACE, AK_NPC, AK_MONSTER, AK_ZONE, AK_ITEMCLASS, AK_ITEMTYPE,
    AK_OPTION, AK_QUESTITEM, AK_QUEST, AK_DOMAIN, AK_SECTIONTYPE, AK_QUESTTYPE, AK_GRADE
};

struct AttrSpec
{
    const wchar_t* name;
    AttrKind       kind;
    bool           required;   // the server Asserts on it
    const wchar_t* sample;     // value put in by "Add element"
    const wchar_t* help;
};

struct ElemSpec
{
    const wchar_t* name;
    ElemGroup      group;
    bool           serverKnown;   // registered in GQuestElementFactory
    bool           hasText;       // the client shows / uses the element text
    const wchar_t* help;
    std::vector<AttrSpec> attrs;
    std::vector<const wchar_t*> children;   // element names it may contain (empty = none expected)
};

const std::vector<ElemSpec>& SchemaElements();
const ElemSpec* SchemaFind(const std::wstring& name);
const wchar_t*  SchemaGroupTitle(ElemGroup g);
// A new element with the sample values filled in.
std::unique_ptr<XNode> SchemaNewElement(const std::wstring& name);
// A new quest with the usual skeleton.
std::unique_ptr<XNode> SchemaNewQuest(long id);
// Element reference as text.
std::wstring SchemaReferenceText();

//----------------------------------------------------------------------
// names next to ids (best effort: every source is optional)
//----------------------------------------------------------------------
struct NameTable
{
    std::map<long, std::wstring> monsters;        // MType -> "Chief Dead Body [1] (Lv 5)"
    std::map<long, std::wstring> itemClasses;     // class id -> SERUM
    std::map<std::wstring, long> itemClassIds;    // SERUM -> class id
    std::map<long, std::map<long, std::wstring>> itemTypes;   // class -> type -> name
    std::map<std::wstring, std::wstring> optionsByNick;        // STR+1 -> Wolf
    std::map<long, std::wstring> zones;           // zone id -> name
    std::map<long, std::wstring> npcs;            // npc id -> name
    long questItemClass = -1;                     // item class id of GQUEST_ITEM, if known
    std::vector<std::wstring> sources;            // files that were read
    std::vector<std::wstring> problems;

    void clear();
    // Looks for Tools\BinEditor\names.json, Docs\Zones.md and
    // ReleaseData\Data\info\npc.en.inf upward from each of `roots`.
    void load(const std::vector<std::wstring>& roots);
    bool loadNamesJson(const std::wstring& path);
    bool loadZonesMd(const std::wstring& path);
    bool loadNpcInf(const std::wstring& path);
    bool empty() const { return monsters.empty() && zones.empty() && npcs.empty() && itemClasses.empty(); }

    // "" when nothing is known
    std::wstring hint(const XNode& n, const std::wstring& attr, const std::wstring& value, const XNode* questList) const;
};

//----------------------------------------------------------------------
// checks
//----------------------------------------------------------------------
enum CheckLevel { CK_ERROR = 0, CK_WARNING, CK_NOTE };

struct CheckItem
{
    CheckLevel   level;
    long         questId;      // -1 for the file as a whole
    const XNode* node;         // where (may be null)
    std::wstring where;        // "Quest 101 > Complete > SayNPC[1]"
    std::wstring message;
};

// clientFile: the copy the client reads (its scanner's limits apply);
// otherwise the server's rules apply. Both sets are always reported, the
// side that matters is marked as an error, the other as a warning.
void SchemaCheck(const XDoc& doc, bool clientFile, const NameTable* names, std::vector<CheckItem>& out);
std::wstring SchemaCheckText(const std::vector<CheckItem>& items);
std::wstring SchemaNodePath(const XNode* n);

// Structural comparison of two quest lists (ignores text, keeps element
// names, index attributes and order).
std::wstring SchemaCompareText(const XDoc& a, const std::wstring& aName, const XDoc& b, const std::wstring& bName);
