// QuestSchema.cpp - see QuestSchema.h
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "QuestSchema.h"

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <fstream>
#include <set>
#include <sstream>

//----------------------------------------------------------------------
// vocabulary
//----------------------------------------------------------------------
namespace {

#define A(name, kind, req, sample, help) AttrSpec{ L##name, kind, req, L##sample, L##help }

const std::vector<ElemSpec>& elements()
{
    static const std::vector<ElemSpec> v = {
        // ---- the quest itself (never offered by "Add element") -------------
        { L"Quest", EG_GROUPS, true, false, L"One quest. id is what the server and the client agree on; the other attributes are read by the client only.",
          { A("id", AK_INT, true, "0", "quest id, unique in the file"),
            A("type", AK_QUESTTYPE, false, "", "tutorial / charge / adventure: journal tab and map icon (needs HappenRegion)"),
            A("autostart", AK_INT, false, "", "1 = the client accepts the quest as soon as it can be taken"),
            A("ending", AK_INT, false, "", "ending event id sent when the quest completes") }, {} },
        // ---- quest level --------------------------------------------------
        { L"Title", EG_QUEST, false, true, L"The quest name in the journal and quest list.", {}, {} },
        { L"Script", EG_QUEST, false, true,
          L"NPC dialog. Under <Quest> it is the offer text (sender = who speaks). Inside <Complete> it is shown "
          L"when the step with that index is reached: target = NPC id for the portrait, or targetitem = quest item type.",
          { A("sender", AK_TEXT, false, "NPC name", "name shown as the speaker"),
            A("target", AK_NPC, false, "", "NPC id whose portrait is shown (Complete only)"),
            A("targetitem", AK_QUESTITEM, false, "", "quest item type whose description is shown instead of an NPC"),
            A("index", AK_INT, false, "", "step number this dialog belongs to (Complete only)"),
            A("action", AK_TEXT, false, "", "\"dead\": the client kills the player when the dialog closes"),
            A("warp", AK_ZONE, false, "", "zone to send the player to with action=\"dead\"") }, {} },
        { L"CompleteMessage", EG_QUEST, false, true, L"What the NPC says when every Complete step is done.",
          { A("sender", AK_TEXT, false, "NPC name", ""), A("target", AK_NPC, false, "0", "NPC id for the portrait") }, {} },
        { L"FailMessage", EG_QUEST, false, true, L"What the NPC says when a Fail condition triggers.",
          { A("sender", AK_TEXT, false, "NPC name", ""), A("target", AK_NPC, false, "0", "NPC id for the portrait") }, {} },
        { L"RewardMessage", EG_QUEST, false, true, L"Free text listing the reward, shown on the quest's detail page (multi-line allowed).", {}, {} },
        { L"HappenRegion", EG_QUEST, false, false, L"Where the quest starts; the client draws the quest icon on the map there (needs the Quest type attribute too).",
          { A("zoneid", AK_ZONE, true, "22", ""), A("x", AK_INT, true, "0", ""), A("y", AK_INT, true, "0", "") }, {} },
        // ---- sections -----------------------------------------------------
        { L"Happen", EG_SECTION, false, false, L"Conditions to be offered the quest. Checked in sequence; type=\"and\"/\"or\"/\"sequence\" overrides.",
          { A("replay", AK_BOOL, false, "false", "true = can be done again"), A("type", AK_SECTIONTYPE, false, "", "and / or / sequence (default sequence)") }, {} },
        { L"Complete", EG_SECTION, false, false, L"Steps, in order, that finish the quest. Dialog (Script) and actions in between run when their turn comes.",
          { A("type", AK_SECTIONTYPE, false, "", "and / or / sequence (default sequence)") }, {} },
        { L"Fail", EG_SECTION, false, false, L"Conditions that fail the quest (any one; default type or).",
          { A("type", AK_SECTIONTYPE, false, "", "and / or / sequence (default or)") }, {} },
        { L"Reward", EG_SECTION, false, false, L"Actions run when the quest completes.",
          { A("type", AK_SECTIONTYPE, false, "", "") }, {} },
        // ---- conditions ---------------------------------------------------
        { L"Level", EG_CONDITION, true, true, L"Player level between min and max (either may be left out). With index and text it is a Complete step.",
          { A("min", AK_INT, false, "1", ""), A("max", AK_INT, false, "", ""), A("index", AK_INT, false, "", "step number when used in Complete") }, {} },
        { L"Race", EG_CONDITION, true, false, L"Player race.", { A("race", AK_RACE, false, "SLAYER", "SLAYER / VAMPIRE / OUSTERS") }, {} },
        { L"Gender", EG_CONDITION, true, false, L"Player gender.", { A("restriction", AK_TEXT, true, "MALE", "MALE / FEMALE") }, {} },
        { L"SkillLevel", EG_CONDITION, true, true, L"Skill level reached (Complete step).", { A("min", AK_INT, false, "3", ""), A("index", AK_INT, false, "1", "") }, {} },
        { L"AdvancementClassLevel", EG_CONDITION, true, false, L"Advancement class level between min and max.",
          { A("min", AK_INT, false, "1", ""), A("max", AK_INT, false, "", ""), A("index", AK_INT, false, "", "") }, {} },
        { L"SlayerHighestSkillDomain", EG_CONDITION, true, false,
          L"Holds when the slayer's highest skill domain is this one. In Reward it wraps the GiveItem for that domain.",
          { A("domain", AK_DOMAIN, true, "sword", "blade / sword / gun / heal / enchant (lowercase, the server compares exactly)") }, { L"GiveItem", L"Random" } },
        { L"HasQuestItem", EG_CONDITION, true, false, L"Player carries the quest item.", { A("type", AK_QUESTITEM, true, "0", "quest item type"), A("num", AK_INT, false, "1", "") }, {} },
        { L"CompleteQuest", EG_CONDITION, true, false, L"Another quest has been completed.", { A("id", AK_QUEST, false, "0", "quest id") }, {} },
        { L"EnterZone", EG_CONDITION, true, true, L"Player is in the zone.", { A("ZoneID", AK_ZONE, false, "0", "") }, {} },
        { L"EnterDynamicZone", EG_CONDITION, true, true, L"Player entered the dynamic zone.", { A("zoneid", AK_ZONE, true, "0", ""), A("index", AK_INT, false, "", "") }, {} },
        { L"ClearDynamicZone", EG_CONDITION, true, true, L"The dynamic zone was cleared.", { A("zoneid", AK_ZONE, true, "0", ""), A("index", AK_INT, false, "", "") }, {} },
        { L"Time", EG_CONDITION, true, true, L"Fails after limit minutes (Fail section). Text may use %s and %d like the client's own strings.",
          { A("limit", AK_INT, true, "30", "minutes"), A("index", AK_INT, false, "1", "") }, {} },
        { L"Killed", EG_CONDITION, true, true, L"Player died num times (Fail section).", { A("num", AK_INT, true, "1", ""), A("index", AK_INT, false, "1", "") }, {} },
        { L"IllegalWarp", EG_CONDITION, true, true, L"Player used a helicopter / illegal warp (Fail section).", { A("index", AK_INT, false, "1", "") }, {} },
        { L"RideMotorcycle", EG_CONDITION, true, true, L"Player rode a motorcycle (Fail section).", { A("index", AK_INT, false, "1", "") }, {} },
        { L"EventParty", EG_CONDITION, true, true, L"Event party condition.", { A("index", AK_INT, false, "", "") }, {} },
        { L"EventPartyCrash", EG_CONDITION, true, true, L"Event party crash condition.", { A("index", AK_INT, false, "", "") }, {} },
        { L"FastMove", EG_CONDITION, true, true, L"Fast move condition.", { A("index", AK_INT, false, "", "") }, {} },
        // ---- steps --------------------------------------------------------
        { L"KillMonster", EG_STEP, true, true, L"Kill num monsters of the Target types. Text: \"%s\" = done mark, \"%d\" = count.",
          { A("num", AK_INT, true, "5", "how many"), A("sort", AK_INT, true, "1", "number of Target children"), A("index", AK_INT, false, "1", "") }, { L"Target" } },
        { L"PartyDissect", EG_STEP, true, true, L"Party kill step: Level ranges with Target children.",
          { A("num", AK_INT, true, "1", ""), A("sort", AK_INT, true, "1", ""), A("index", AK_INT, false, "1", "") }, { L"Level" } },
        { L"BloodDrain", EG_STEP, true, true, L"Blood drain num times.", { A("num", AK_INT, true, "20", ""), A("index", AK_INT, false, "1", "") }, {} },
        { L"SayNPC", EG_STEP, true, true, L"Talk to NPC target. name = shown as the speaker by the client.",
          { A("name", AK_TEXT, false, "NPC name", "speaker name (client)"), A("target", AK_NPC, true, "0", "NPC id"), A("index", AK_INT, false, "1", ""), A("volume", AK_INT, false, "", "") }, {} },
        { L"TouchWayPoint", EG_STEP, true, true, L"Reach the spot (zoneid, x, y). dir = facing, type = object type.",
          { A("zoneid", AK_ZONE, true, "22", ""), A("x", AK_INT, true, "0", ""), A("y", AK_INT, true, "0", ""), A("dir", AK_INT, true, "1", ""), A("type", AK_INT, false, "", "object type to touch"), A("index", AK_INT, false, "1", "") }, {} },
        { L"Travel", EG_STEP, true, true, L"Travel the ways of grade (TravelWay.xml) starting at basenum.",
          { A("basenum", AK_INT, true, "1", ""), A("grade", AK_GRADE, true, "A", "A / B / C / D"), A("index", AK_INT, false, "1", "") }, {} },
        { L"TamePet", EG_STEP, true, true, L"Tame a pet.", { A("num", AK_INT, false, "1", ""), A("index", AK_INT, false, "1", "") }, {} },
        // ---- actions ------------------------------------------------------
        { L"GiveItem", EG_ACTION, true, false, L"Give an item: class = item class name, type = type in that class.",
          { A("class", AK_ITEMCLASS, true, "SERUM", "item class name"), A("type", AK_ITEMTYPE, true, "0", ""), A("num", AK_INT, false, "", "default 1"),
            A("option", AK_OPTION, false, "", "option nick, e.g. STR+1"), A("limitedtime", AK_INT, false, "", "seconds") }, {} },
        { L"GiveQuestItem", EG_ACTION, true, false, L"Put a quest item in the quest inventory.", { A("id", AK_QUESTITEM, false, "0", "quest item type"), A("save", AK_BOOL, false, "", "") }, {} },
        { L"LoseQuestItem", EG_ACTION, true, false, L"Take the quest item away.", { A("id", AK_QUESTITEM, false, "0", "quest item type") }, {} },
        { L"GiveEventQuestItem", EG_ACTION, true, false, L"Give an event quest item.", { A("grade", AK_INT, true, "1", ""), A("type", AK_INT, true, "0", "") }, {} },
        { L"GiveMoney", EG_ACTION, true, false, L"Give money.", { A("amount", AK_INT, false, "1000", "") }, {} },
        { L"GiveVampireExp", EG_ACTION, true, false, L"Vampire experience.", { A("amount", AK_INT, false, "100", "") }, {} },
        { L"GiveOustersExp", EG_ACTION, true, false, L"Ousters experience.", { A("amount", AK_INT, false, "100", "") }, {} },
        { L"GiveDomainExp", EG_ACTION, true, false, L"Slayer domain experience.", { A("amount", AK_INT, false, "100", "") }, {} },
        { L"GiveAdvancementClassExp", EG_ACTION, true, false, L"Advancement class experience.", { A("amount", AK_INT, false, "100", "") }, {} },
        { L"AdvanceClass", EG_ACTION, true, false, L"Advance the player's class.", {}, {} },
        { L"AddEffect", EG_ACTION, true, false, L"Put an effect on the player.", { A("effectclass", AK_INT, true, "0", "effect class id") }, {} },
        { L"RemoveEffect", EG_ACTION, true, false, L"Remove the effect.", { A("effectclass", AK_INT, true, "0", "effect class id") }, {} },
        { L"Warp", EG_ACTION, true, false, L"Move the player.", { A("zoneid", AK_ZONE, true, "0", ""), A("x", AK_INT, true, "0", ""), A("y", AK_INT, true, "0", "") }, {} },
        { L"StartOtherQuest", EG_ACTION, true, false, L"Start another quest.", { A("id", AK_QUEST, true, "0", "quest id") }, {} },
        { L"SetDynamicZone", EG_ACTION, true, false, L"Allow or forbid entering a dynamic zone.", { A("zoneid", AK_ZONE, true, "0", ""), A("canenter", AK_BOOL, true, "true", "") }, {} },
        // ---- client-only text --------------------------------------------
        { L"Message", EG_TEXT, false, true, L"Client-only: a line shown when the step with this index is reached (sender/target make it an NPC dialog).",
          { A("sender", AK_TEXT, false, "", ""), A("target", AK_NPC, false, "", ""), A("index", AK_INT, false, "1", "") }, {} },
        // ---- containers ---------------------------------------------------
        { L"NOT", EG_CONTAINER, true, false, L"Holds when none of its children hold.", {}, {} },
        { L"OR", EG_CONTAINER, true, false, L"Holds when any child holds; in Reward, runs its children.", {}, {} },
        { L"Random", EG_CONTAINER, true, false, L"Runs one of its children at random.", {}, {} },
        { L"Target", EG_CONTAINER, false, false, L"A monster type inside KillMonster / PartyDissect.", { A("type", AK_MONSTER, true, "0", "monster MType") }, {} },
    };
    return v;
}
#undef A

const wchar_t* raceNames[] = { L"SLAYER", L"VAMPIRE", L"OUSTERS" };
const wchar_t* sectionNames[] = { L"Happen", L"Complete", L"Fail", L"Reward" };
const wchar_t* questTypeNames[] = { L"tutorial", L"charge", L"adventure" };   // the client's m_strQuestType
// GQuestSlayerHighestSkillDomainElement compares these exact lowercase spellings
const wchar_t* domainNames[] = { L"blade", L"sword", L"gun", L"heal", L"enchant" };

bool isSection(const std::wstring& n) { for (auto s : sectionNames) if (n == s) return true; return false; }
bool isInt(const std::wstring& v)
{
    std::wstring t = XTrim(v);
    if (t.empty()) return false;
    size_t i = (t[0] == L'-' || t[0] == L'+') ? 1 : 0;
    if (i >= t.size()) return false;
    for (; i < t.size(); i++) if (!iswdigit(t[i])) return false;
    return true;
}
std::wstring lower(std::wstring s) { for (auto& c : s) c = (wchar_t)towlower(c); return s; }

} // namespace

const std::vector<ElemSpec>& SchemaElements() { return elements(); }

const ElemSpec* SchemaFind(const std::wstring& name)
{
    for (const auto& e : elements()) if (name == e.name) return &e;
    return nullptr;
}

const wchar_t* SchemaGroupTitle(ElemGroup g)
{
    switch (g)
    {
        case EG_QUEST:     return L"Quest text and region";
        case EG_SECTION:   return L"Sections";
        case EG_CONDITION: return L"Conditions";
        case EG_STEP:      return L"Steps";
        case EG_ACTION:    return L"Actions";
        case EG_TEXT:      return L"Client dialog";
        case EG_CONTAINER: return L"Containers";
        default:           return L"";
    }
}

std::unique_ptr<XNode> SchemaNewElement(const std::wstring& name)
{
    auto n = std::make_unique<XNode>();
    n->name = name;
    const ElemSpec* s = SchemaFind(name);
    if (s)
    {
        for (const auto& a : s->attrs)
            if (a.sample[0]) n->attrs.push_back({ a.name, a.sample });
        if (s->hasText && s->group != EG_QUEST && s->group != EG_TEXT) n->text = L"Describe this step";
        else if (name == L"Title") n->text = L"New quest";
        else if (s->hasText) n->text = L"Text";
        if (name == L"KillMonster")
        {
            XNode* t = n->addChild(L"Target");
            t->attrs.push_back({ L"type", L"0" });
        }
    }
    return n;
}

std::unique_ptr<XNode> SchemaNewQuest(long id)
{
    auto q = std::make_unique<XNode>();
    q->name = L"Quest";
    q->attrs.push_back({ L"id", std::to_wstring(id) });
    q->addChild(L"Title")->text = L"New quest";
    XNode* s = q->addChild(L"Script");
    s->attrs.push_back({ L"sender", L"NPC name" });
    s->text = L"What the NPC says when the quest is offered.";
    XNode* h = q->addChild(L"Happen");
    h->attrs.push_back({ L"replay", L"false" });
    h->addChild(L"Race")->attrs.push_back({ L"race", L"SLAYER" });
    XNode* lv = h->addChild(L"Level");
    lv->attrs.push_back({ L"min", L"1" });
    lv->attrs.push_back({ L"max", L"10" });
    XNode* cm = q->addChild(L"CompleteMessage");
    cm->attrs.push_back({ L"sender", L"NPC name" });
    cm->attrs.push_back({ L"target", L"0" });
    cm->text = L"What the NPC says when the quest is finished.";
    XNode* c = q->addChild(L"Complete");
    XNode* step = c->addChild(L"Level");
    step->attrs.push_back({ L"min", L"2" });
    step->attrs.push_back({ L"index", L"1" });
    step->text = L"Reach level 2";
    XNode* fm = q->addChild(L"FailMessage");
    fm->attrs.push_back({ L"sender", L"NPC name" });
    fm->attrs.push_back({ L"target", L"0" });
    fm->text = L"What the NPC says when the quest fails.";
    q->addChild(L"Fail");
    XNode* r = q->addChild(L"Reward");
    r->addChild(L"GiveMoney")->attrs.push_back({ L"amount", L"1000" });
    return q;
}

std::wstring SchemaReferenceText()
{
    std::wstring t;
    t += L"QUEST FILE REFERENCE\r\n\r\n";
    t += L"<QuestList>\r\n  <Quest id=\"101\" type=\"tutorial\" autostart=\"1\" ending=\"0\">   (type/autostart/ending are read by the client only;\r\n";
    t += L"      type = tutorial / charge / adventure puts the quest in that journal tab and on the map with HappenRegion)\r\n";
    t += L"    <Title>, <Script sender>, <Happen>, <CompleteMessage>, <Complete>, <FailMessage>, <Fail>, <Reward>, <RewardMessage>, <HappenRegion>\r\n";
    t += L"  </Quest>\r\n</QuestList>\r\n\r\n";
    t += L"Every quest needs all four sections (Happen, Complete, Fail, Reward): the server dereferences a missing one and crashes at boot.\r\n";
    t += L"Steps in Complete and Fail carry index=\"n\": the server reports progress by index, the client finds the step text\r\n";
    t += L"(and any Script / Message dialog with the same index) by it. Keep indexes unique inside a section.\r\n\r\n";
    for (int g = 0; g < EG_GROUPS; g++)
    {
        t += std::wstring(L"== ") + SchemaGroupTitle((ElemGroup)g) + L" ==\r\n";
        for (const auto& e : elements())
        {
            if (e.group != g || e.name == std::wstring(L"Quest")) continue;
            t += L"  <" + std::wstring(e.name);
            for (const auto& a : e.attrs) t += std::wstring(L" ") + a.name + (a.required ? L"=\"!\"" : L"=\"\"");
            t += L">";
            if (e.hasText) t += L" text";
            if (!e.serverKnown) t += L"   [client only]";
            t += L"\r\n      " + std::wstring(e.help) + L"\r\n";
            for (const auto& a : e.attrs)
                if (a.help[0]) t += std::wstring(L"        ") + a.name + L": " + a.help + L"\r\n";
        }
        t += L"\r\n";
    }
    t += L"=\"!\" marks an attribute the server insists on.\r\n";
    return t;
}

//----------------------------------------------------------------------
// names
//----------------------------------------------------------------------
namespace {

// a very small JSON reader, enough for names.json
struct JVal
{
    enum T { NUL, BOOL, NUM, STR, ARR, OBJ } t = NUL;
    bool b = false;
    double num = 0;
    std::wstring str;
    std::vector<JVal> arr;
    std::vector<std::pair<std::wstring, JVal>> obj;
    const JVal* get(const wchar_t* k) const
    {
        for (const auto& p : obj) if (p.first == k) return &p.second;
        return nullptr;
    }
};

struct JParser
{
    const std::wstring& s;
    size_t i = 0;
    bool ok = true;
    void ws() { while (i < s.size() && iswspace(s[i])) i++; }
    bool value(JVal& v)
    {
        ws();
        if (i >= s.size()) return ok = false;
        wchar_t c = s[i];
        if (c == L'{')
        {
            v.t = JVal::OBJ; i++;
            while (true)
            {
                ws();
                if (i < s.size() && s[i] == L'}') { i++; return true; }
                JVal k;
                if (!value(k) || k.t != JVal::STR) return ok = false;
                ws();
                if (i >= s.size() || s[i] != L':') return ok = false;
                i++;
                JVal val;
                if (!value(val)) return false;
                v.obj.emplace_back(k.str, std::move(val));
                ws();
                if (i < s.size() && s[i] == L',') { i++; continue; }
                if (i < s.size() && s[i] == L'}') { i++; return true; }
                return ok = false;
            }
        }
        if (c == L'[')
        {
            v.t = JVal::ARR; i++;
            while (true)
            {
                ws();
                if (i < s.size() && s[i] == L']') { i++; return true; }
                JVal e;
                if (!value(e)) return false;
                v.arr.push_back(std::move(e));
                ws();
                if (i < s.size() && s[i] == L',') { i++; continue; }
                if (i < s.size() && s[i] == L']') { i++; return true; }
                return ok = false;
            }
        }
        if (c == L'"')
        {
            v.t = JVal::STR; i++;
            while (i < s.size() && s[i] != L'"')
            {
                if (s[i] == L'\\' && i + 1 < s.size())
                {
                    i++;
                    wchar_t e = s[i];
                    if (e == L'n') v.str += L'\n';
                    else if (e == L't') v.str += L'\t';
                    else if (e == L'r') v.str += L'\r';
                    else if (e == L'u' && i + 4 < s.size()) { v.str += (wchar_t)wcstoul(s.substr(i + 1, 4).c_str(), nullptr, 16); i += 4; }
                    else v.str += e;
                    i++;
                }
                else v.str += s[i++];
            }
            if (i >= s.size()) return ok = false;
            i++;
            return true;
        }
        if (s.compare(i, 4, L"true") == 0) { v.t = JVal::BOOL; v.b = true; i += 4; return true; }
        if (s.compare(i, 5, L"false") == 0) { v.t = JVal::BOOL; i += 5; return true; }
        if (s.compare(i, 4, L"null") == 0) { i += 4; return true; }
        wchar_t* end = nullptr;
        v.num = wcstod(s.c_str() + i, &end);
        if (!end || end == s.c_str() + i) return ok = false;
        v.t = JVal::NUM;
        i = (size_t)(end - s.c_str());
        return true;
    }
};

bool readAll(const std::wstring& path, std::vector<uint8_t>& out)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz;
    GetFileSizeEx(h, &sz);
    out.resize((size_t)sz.QuadPart);
    DWORD got = 0;
    bool ok = out.empty() || (ReadFile(h, out.data(), (DWORD)out.size(), &got, nullptr) && got == out.size());
    CloseHandle(h);
    return ok;
}

bool fileExists(const std::wstring& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring dirOf(const std::wstring& p)
{
    size_t s = p.find_last_of(L"\\/");
    return s == std::wstring::npos ? L"" : p.substr(0, s);
}

std::wstring fmt(const wchar_t* f, ...)
{
    wchar_t buf[1024];
    va_list ap;
    va_start(ap, f);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, f, ap);
    va_end(ap);
    return buf;
}

} // namespace

void NameTable::clear() { *this = NameTable(); }

void NameTable::load(const std::vector<std::wstring>& roots)
{
    clear();
    std::set<std::wstring> tried;
    bool gotJson = false, gotZones = false, gotNpc = false;
    for (const std::wstring& start : roots)
    {
        std::wstring dir = start;
        for (int up = 0; up < 8 && !dir.empty(); up++)
        {
            if (!tried.insert(lower(dir)).second) break;
            std::wstring j = dir + L"\\Tools\\BinEditor\\names.json";
            std::wstring z = dir + L"\\Docs\\Zones.md";
            std::wstring n1 = dir + L"\\ReleaseData\\Data\\info\\npc.en.inf";
            std::wstring n2 = dir + L"\\ReleaseData\\Data\\info\\npc.inf";
            std::wstring n3 = dir + L"\\Release\\Data\\Info\\npc.en.inf";
            if (!gotJson && fileExists(j)) gotJson = loadNamesJson(j);
            if (!gotZones && fileExists(z)) gotZones = loadZonesMd(z);
            if (!gotNpc)
            {
                if (fileExists(n1)) gotNpc = loadNpcInf(n1);
                else if (fileExists(n2)) gotNpc = loadNpcInf(n2);
                else if (fileExists(n3)) gotNpc = loadNpcInf(n3);
            }
            // names.json next to the exe too
            std::wstring j2 = dir + L"\\names.json";
            if (!gotJson && fileExists(j2)) gotJson = loadNamesJson(j2);
            dir = dirOf(dir);
        }
    }
}

bool NameTable::loadNamesJson(const std::wstring& path)
{
    std::vector<uint8_t> bytes;
    if (!readAll(path, bytes)) { problems.push_back(L"cannot read " + path); return false; }
    size_t off = (bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) ? 3 : 0;
    std::wstring text = XBytesToWide(bytes.data() + off, bytes.size() - off, XENC_UTF8);
    JVal root;
    JParser p{ text };
    if (!p.value(root) || root.t != JVal::OBJ) { problems.push_back(path + L": not readable as JSON"); return false; }

    if (const JVal* mons = root.get(L"monsters"))
        for (const JVal& m : mons->arr)
        {
            const JVal* t = m.get(L"t");
            const JVal* e = m.get(L"e");
            const JVal* lvl = m.get(L"lvl");
            if (!t || t->t != JVal::NUM) continue;
            std::wstring name = e && e->t == JVal::STR ? e->str : L"?";
            if (lvl && lvl->t == JVal::NUM) name += fmt(L" (Lv %d)", (int)lvl->num);
            monsters[(long)t->num] = name;
        }
    if (const JVal* cls = root.get(L"item_classes"))
        for (const auto& kv : cls->obj)
            if (kv.second.t == JVal::STR)
            {
                long id = wcstol(kv.first.c_str(), nullptr, 10);
                itemClasses[id] = kv.second.str;
                itemClassIds[kv.second.str] = id;
                // ITEM_CLASS_GQUEST_ITEM is the class quest items (GiveQuestItem id=...) live in
                if (kv.second.str == L"GQUEST_ITEM" || (kv.second.str == L"QUEST_ITEM" && questItemClass < 0)) questItemClass = id;
            }
    if (const JVal* items = root.get(L"items"))
        for (const auto& kv : items->obj)
        {
            long cid = wcstol(kv.first.c_str(), nullptr, 10);
            const JVal* types = kv.second.get(L"types");
            if (!types) continue;
            for (const auto& tv : types->obj)
            {
                const JVal* e = tv.second.get(L"e");
                if (e && e->t == JVal::STR) itemTypes[cid][wcstol(tv.first.c_str(), nullptr, 10)] = e->str;
            }
        }
    if (const JVal* opts = root.get(L"options"))
        for (const auto& kv : opts->obj)
        {
            const JVal* e = kv.second.get(L"e");
            const JVal* nick = kv.second.get(L"nick");
            if (e && e->t == JVal::STR && nick && nick->t == JVal::STR) optionsByNick[nick->str] = e->str + L" (option " + kv.first + L")";
        }
    sources.push_back(path);
    return true;
}

bool NameTable::loadZonesMd(const std::wstring& path)
{
    std::vector<uint8_t> bytes;
    if (!readAll(path, bytes)) return false;
    size_t off = (bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) ? 3 : 0;
    std::wstring text = XBytesToWide(bytes.data() + off, bytes.size() - off, XENC_UTF8);
    std::wistringstream in(text);
    std::wstring line;
    int zoneCol = -1;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] != L'|') continue;
        std::vector<std::wstring> cells;
        size_t p = 1;
        while (p <= line.size())
        {
            size_t e = line.find(L'|', p);
            if (e == std::wstring::npos) break;
            cells.push_back(XTrim(line.substr(p, e - p)));
            p = e + 1;
        }
        if (cells.empty()) continue;
        if (cells[0] == L"ID")
        {
            zoneCol = -1;
            for (size_t k = 1; k < cells.size(); k++) if (cells[k] == L"Zone") zoneCol = (int)k;
            continue;
        }
        if (zoneCol < 0 || (size_t)zoneCol >= cells.size() || !isInt(cells[0])) continue;
        long id = wcstol(cells[0].c_str(), nullptr, 10);
        if (zones.find(id) == zones.end()) zones[id] = cells[(size_t)zoneCol];
    }
    if (!zones.empty()) sources.push_back(path);
    return !zones.empty();
}

bool NameTable::loadNpcInf(const std::wstring& path)
{
    std::vector<uint8_t> b;
    if (!readAll(path, b)) return false;
    size_t i = 0;
    auto u32 = [&](uint32_t& v) { if (i + 4 > b.size()) return false; v = b[i] | (b[i + 1] << 8) | (b[i + 2] << 16) | ((uint32_t)b[i + 3] << 24); i += 4; return true; };
    auto str = [&](std::wstring& s) { uint32_t len; if (!u32(len) || len > 0x10000 || i + len > b.size()) return false; s = XBytesToWide(b.data() + i, len, XENC_CP949); i += len; return true; };
    uint32_t count;
    if (!u32(count) || count > 100000) return false;
    std::map<long, std::wstring> found;
    for (uint32_t k = 0; k < count; k++)
    {
        uint32_t id, n, tmp;
        std::wstring name, desc;
        if (!u32(id) || !str(name) || !u32(n) || n > 1000) return false;
        for (uint32_t j = 0; j < n; j++) if (!u32(tmp)) return false;
        if (!str(desc) || !u32(tmp)) return false;
        found[(long)id] = name;
    }
    npcs = found;
    sources.push_back(path);
    return true;
}

std::wstring NameTable::hint(const XNode& n, const std::wstring& attr, const std::wstring& value, const XNode* questList) const
{
    const ElemSpec* spec = SchemaFind(n.name);
    AttrKind kind = AK_TEXT;
    if (spec) for (const auto& a : spec->attrs) if (attr == a.name) kind = a.kind;
    std::wstring v = XTrim(value);
    long iv = isInt(v) ? wcstol(v.c_str(), nullptr, 10) : -1;
    auto lookup = [&](const std::map<long, std::wstring>& m, const wchar_t* what) -> std::wstring {
        if (iv < 0) return L"";
        auto it = m.find(iv);
        if (it == m.end()) return m.empty() ? L"" : fmt(L"[no %s %ld in the name tables]", what, iv);
        return L"[" + it->second + L"]";
    };
    switch (kind)
    {
        case AK_NPC:     return lookup(npcs, L"NPC");
        case AK_MONSTER: return lookup(monsters, L"monster");
        case AK_ZONE:    return lookup(zones, L"zone");
        case AK_ITEMCLASS:
        {
            if (iv >= 0) return lookup(itemClasses, L"item class");
            if (itemClassIds.empty()) return L"";
            auto it = itemClassIds.find(v);
            return it == itemClassIds.end() ? L"[unknown item class name]" : fmt(L"[class %ld]", it->second);
        }
        case AK_ITEMTYPE:
        {
            std::wstring cls = n.attrValue(L"class");
            long cid = isInt(cls) ? wcstol(cls.c_str(), nullptr, 10) : -1;
            if (cid < 0) { auto it = itemClassIds.find(XTrim(cls)); if (it != itemClassIds.end()) cid = it->second; }
            if (cid < 0 || iv < 0) return L"";
            auto ct = itemTypes.find(cid);
            if (ct == itemTypes.end()) return L"";
            auto tt = ct->second.find(iv);
            return tt == ct->second.end() ? fmt(L"[no type %ld in that class]", iv) : L"[" + tt->second + L"]";
        }
        case AK_OPTION:
        {
            if (v.empty() || optionsByNick.empty()) return L"";
            auto it = optionsByNick.find(v);
            return it == optionsByNick.end() ? L"[unknown option nick]" : L"[" + it->second + L"]";
        }
        case AK_QUESTITEM:
        {
            if (questItemClass < 0 || iv < 0) return L"";
            auto ct = itemTypes.find(questItemClass);
            if (ct == itemTypes.end()) return L"";
            auto tt = ct->second.find(iv);
            return tt == ct->second.end() ? L"" : L"[" + tt->second + L"]";
        }
        case AK_QUEST:
        {
            if (!questList || iv < 0) return L"";
            for (const auto& q : questList->children)
                if (q->name == L"Quest" && q->attrInt(L"id") == iv)
                {
                    const XNode* t = q->child(L"Title");
                    return L"[" + (t ? XOneLine(t->text, 40) : std::wstring(L"quest")) + L"]";
                }
            return L"[no quest with that id in this file]";
        }
        default: return L"";
    }
}

//----------------------------------------------------------------------
// checks
//----------------------------------------------------------------------
std::wstring SchemaNodePath(const XNode* n)
{
    std::vector<std::wstring> parts;
    for (const XNode* p = n; p && p->parent; p = p->parent)
    {
        std::wstring s = p->name;
        if (p->name == L"Quest") s += L" " + p->attrValue(L"id", L"?");
        else
        {
            long idx = p->attrInt(L"index");
            if (idx >= 0) s += L"[" + std::to_wstring(idx) + L"]";
        }
        parts.push_back(s);
    }
    std::wstring o;
    for (size_t i = parts.size(); i-- > 0;) { o += parts[i]; if (i) o += L" > "; }
    return o;
}

namespace {

struct Checker
{
    const XDoc& doc;
    bool clientFile;
    const NameTable* names;
    std::vector<CheckItem>& out;
    long qid = -1;

    void add(CheckLevel lvl, const XNode* n, const std::wstring& msg)
    {
        out.push_back({ lvl, qid, n, n ? SchemaNodePath(n) : L"file", msg });
    }
    // a server rule: an error in the server copy, a warning in the client copy
    void server(const XNode* n, const std::wstring& msg) { add(clientFile ? CK_WARNING : CK_ERROR, n, msg + (clientFile ? L" (server rule)" : L"")); }
    // a client rule: the reverse
    void client(const XNode* n, const std::wstring& msg) { add(clientFile ? CK_ERROR : CK_WARNING, n, msg + (clientFile ? L"" : L" (client rule)")); }

    void checkText(const XNode* n)
    {
        if (n->text.empty()) return;
        if (n->text.find(L'<') != std::wstring::npos)
            client(n, L"Text contains '<': the client's XML reader would take it for a tag.");
        static const wchar_t* trimChars = L" </>\r\t\n=\"'";
        if (wcschr(trimChars, n->text[0]))
            client(n, fmt(L"Text starts with '%c', which the client's XML reader strips off.", n->text[0]));
        if (n->text.find(L'&') != std::wstring::npos && clientFile)
            add(CK_NOTE, n, L"Text contains '&': the client shows it as-is (no entity decoding), the server would decode an entity.");
        if (doc.encoding == XENC_UTF8 && clientFile)
            for (wchar_t ch : n->text)
                if (ch > 127) { add(CK_WARNING, n, L"Non-ASCII text in a UTF-8 file: the client does not decode UTF-8, it reads text as CP949 (euc-kr). Save the file as euc-kr or use ASCII."); break; }
    }

    void checkAttrs(const XNode* n, const ElemSpec* spec)
    {
        std::set<std::wstring> seen;
        for (const auto& a : n->attrs)
        {
            if (!seen.insert(a.name).second) add(CK_ERROR, n, L"Attribute " + a.name + L" appears twice.");
            if (a.value.find(L'"') != std::wstring::npos || a.value.find(L'\'') != std::wstring::npos)
                client(n, L"Attribute " + a.name + L" contains a quote: the client's XML reader ends the value there.");
            if (a.value.find(L'>') != std::wstring::npos || a.value.find(L'<') != std::wstring::npos)
                client(n, L"Attribute " + a.name + L" contains '<' or '>': the client's XML reader cannot parse that.");
            if (XTrim(a.value) != a.value) add(CK_WARNING, n, L"Attribute " + a.name + L" has leading or trailing spaces.");
            if (!spec) continue;
            const AttrSpec* as = nullptr;
            for (const auto& s : spec->attrs) if (a.name == s.name) as = &s;
            if (!as)
            {
                if (spec->serverKnown || spec->group == EG_SECTION)
                    add(CK_NOTE, n, L"Attribute " + a.name + L" is not one <" + n->name + L"> is known to read.");
                continue;
            }
            std::wstring v = XTrim(a.value);
            switch (as->kind)
            {
                case AK_INT: case AK_NPC: case AK_MONSTER: case AK_ZONE: case AK_ITEMTYPE: case AK_QUESTITEM: case AK_QUEST:
                    if (!isInt(v)) add(CK_ERROR, n, L"Attribute " + a.name + L"=\"" + a.value + L"\" should be a whole number.");
                    break;
                case AK_BOOL:
                    if (v != L"true" && v != L"false" && v != L"0" && v != L"1")
                        add(CK_WARNING, n, L"Attribute " + a.name + L"=\"" + a.value + L"\" is not true/false.");
                    break;
                case AK_RACE:
                {
                    bool ok = false;
                    for (auto r : raceNames) if (v == r) ok = true;
                    if (!ok) add(CK_ERROR, n, L"race=\"" + a.value + L"\" is not SLAYER, VAMPIRE or OUSTERS.");
                    break;
                }
                case AK_SECTIONTYPE:
                    if (!v.empty() && v != L"and" && v != L"or" && v != L"sequence")
                        server(n, L"type=\"" + a.value + L"\" must be and, or or sequence.");
                    break;
                case AK_GRADE:
                    if (v.size() != 1 || v[0] < L'A' || v[0] > L'D') add(CK_WARNING, n, L"grade=\"" + a.value + L"\" is usually A, B, C or D.");
                    break;
                case AK_DOMAIN:
                {
                    bool ok = false;
                    for (auto d : domainNames) if (v == d) ok = true;
                    if (!ok) server(n, L"domain=\"" + a.value + L"\" is not blade, sword, gun, heal or enchant (lowercase); the server treats it as no domain.");
                    break;
                }
                case AK_ITEMCLASS:
                    if (names && !names->itemClassIds.empty() && !isInt(v) && names->itemClassIds.find(v) == names->itemClassIds.end())
                        add(CK_ERROR, n, L"class=\"" + a.value + L"\" is not an item class name in the name tables.");
                    break;
                default: break;
            }
            if (names)
            {
                std::wstring h = names->hint(*n, a.name, a.value, doc.root.get());
                if (h.rfind(L"[no ", 0) == 0 || h.rfind(L"[unknown", 0) == 0) add(CK_WARNING, n, a.name + L"=\"" + a.value + L"\" " + h);
            }
        }
        if (spec)
            for (const auto& s : spec->attrs)
                if (s.required && !n->attr(s.name)) server(n, L"<" + n->name + L"> needs the attribute " + s.name + L" (a debug server asserts on it, a release server reads an unset value).");
    }

    void checkElement(const XNode* n, const XNode* section, int depth)
    {
        const ElemSpec* spec = SchemaFind(n->name);
        if (!spec)
        {
            add(CK_WARNING, n, L"<" + n->name + L"> is not an element the server or the client knows; the server skips it.");
        }
        else if (!spec->serverKnown && spec->group != EG_QUEST && spec->group != EG_SECTION && n->name != L"Target")
        {
            // Script / Message inside a section: fine, client only
        }
        checkAttrs(n, spec);
        checkText(n);

        if (n->name == L"KillMonster" || n->name == L"PartyDissect")
        {
            long targets = 0;
            for (const auto& c : n->children)
            {
                if (n->name == L"KillMonster" && c->name != L"Target") server(c.get(), L"<KillMonster> may only contain <Target> children.");
                if (n->name == L"PartyDissect" && c->name != L"Level") server(c.get(), L"<PartyDissect> may only contain <Level> children (each with <Target>s).");
                if (c->name == L"Target") targets++;
            }
            if (n->name == L"KillMonster" && targets == 0) server(n, L"<KillMonster> has no <Target type=\"...\"> child.");
            if (n->name == L"KillMonster" && n->attrInt(L"sort") >= 0 && n->attrInt(L"sort") != targets)
                add(CK_WARNING, n, fmt(L"sort=\"%ld\" but there are %ld <Target> children.", n->attrInt(L"sort"), targets));
        }
        else if (spec && spec->children.empty() && spec->group != EG_CONTAINER && spec->group != EG_SECTION && !n->children.empty())
            add(CK_WARNING, n, L"<" + n->name + L"> is not expected to have child elements.");

        if (n->name == L"Level" && n->attrInt(L"min") >= 0 && n->attrInt(L"max") >= 0 && n->attrInt(L"min") > n->attrInt(L"max"))
            add(CK_ERROR, n, L"Level min is greater than max.");

        if (section && (section->name == L"Complete" || section->name == L"Fail") && spec && (spec->group == EG_STEP || spec->group == EG_CONDITION) && depth == 1)
        {
            if (!n->attr(L"index"))
                client(n, L"Step <" + n->name + L"> has no index attribute; the client cannot show its progress or text.");
            if (spec->hasText && n->text.empty())
                client(n, L"Step <" + n->name + L"> has no text; the journal shows an empty line for it.");
        }
        if ((n->name == L"Script" || n->name == L"Message") && section && depth == 1 && !n->attr(L"index"))
            client(n, L"<" + n->name + L"> inside <" + section->name + L"> has no index; the client never shows it.");

        for (const auto& c : n->children) checkElement(c.get(), section, depth + 1);
    }

    void checkQuest(const XNode* q)
    {
        qid = q->attrInt(L"id");
        if (!q->attr(L"id") || qid < 0) server(q, L"<Quest> has no numeric id attribute.");
        for (const auto& a : q->attrs)
        {
            if (a.name == L"id") continue;
            if (a.name == L"type")
            {
                bool ok = false;
                for (auto t : questTypeNames) if (XTrim(a.value) == t) ok = true;
                if (!ok) add(CK_WARNING, q, L"type=\"" + a.value + L"\" is not tutorial, charge or adventure; the client ignores it.");
                if (!q->child(L"HappenRegion")) add(CK_WARNING, q, L"A typed quest needs <HappenRegion zoneid x y> for the map icon.");
            }
            else if (a.name != L"autostart" && a.name != L"ending")
                add(CK_NOTE, q, L"Attribute " + a.name + L" on <Quest> is not read by the server or the client.");
        }
        for (auto s : sectionNames)
            if (!q->child(s)) server(q, std::wstring(L"Missing <") + s + L"> section: the server dereferences it at boot and crashes.");
        int sectionCount[4] = { 0, 0, 0, 0 };
        for (const auto& c : q->children)
            for (int k = 0; k < 4; k++) if (c->name == sectionNames[k]) sectionCount[k]++;
        for (int k = 0; k < 4; k++)
            if (sectionCount[k] > 1) add(CK_ERROR, q, std::wstring(L"<") + sectionNames[k] + L"> appears more than once.");
        if (!q->child(L"Title")) client(q, L"No <Title>: the quest list shows an empty name.");
        if (!q->child(L"Script")) client(q, L"No <Script>: the quest has no offer dialog / description.");

        for (const auto& c : q->children)
        {
            const ElemSpec* spec = SchemaFind(c->name);
            if (isSection(c->name))
            {
                checkAttrs(c.get(), spec);
                std::map<long, const XNode*> indexes;
                for (const auto& e : c->children)
                {
                    checkElement(e.get(), c.get(), 1);
                    long idx = e->attrInt(L"index");
                    if (idx < 0) continue;
                    const ElemSpec* es = SchemaFind(e->name);
                    bool dialog = e->name == L"Script" || e->name == L"Message";
                    if (dialog) continue;   // dialog shares the index of its step
                    auto it = indexes.find(idx);
                    if (it != indexes.end())
                        client(e.get(), fmt(L"index=\"%ld\" is also used by <%s> in the same section; the client shows the first one only.", idx, it->second->name.c_str()));
                    else indexes[idx] = e.get();
                    (void)es;
                }
                // the server numbers steps by index too: warn on gaps
                if (!indexes.empty() && (c->name == L"Complete" || c->name == L"Fail"))
                {
                    long expect = 1;
                    for (const auto& kv : indexes)
                    {
                        if (kv.first != expect) { add(CK_NOTE, c.get(), fmt(L"Step indexes in <%s> are not 1, 2, 3...: found %ld where %ld was expected.", c->name.c_str(), kv.first, expect)); break; }
                        expect++;
                    }
                }
            }
            else
            {
                if (!spec || spec->group != EG_QUEST)
                    add(CK_WARNING, c.get(), L"<" + c->name + L"> directly under <Quest> is not something the server or the client reads there.");
                checkAttrs(c.get(), spec);
                checkText(c.get());
                for (const auto& e : c->children) checkElement(e.get(), nullptr, 1);
            }
        }
        qid = -1;
    }

    void run()
    {
        if (!doc.root) { add(CK_ERROR, nullptr, L"The file has no root element."); return; }
        if (doc.root->name != L"QuestList") add(CK_ERROR, doc.root.get(), L"The root element is <" + doc.root->name + L">, not <QuestList>.");
        for (const auto& w : doc.warnings) add(CK_WARNING, nullptr, w);
        std::map<long, const XNode*> ids;
        for (const auto& c : doc.root->children)
        {
            if (c->name != L"Quest") { add(CK_WARNING, c.get(), L"<" + c->name + L"> under <QuestList> is ignored; only <Quest> is read."); continue; }
            long id = c->attrInt(L"id");
            if (id >= 0)
            {
                auto it = ids.find(id);
                if (it != ids.end()) { qid = id; add(CK_ERROR, c.get(), fmt(L"Quest id %ld is used twice; the server keeps the last one.", id)); qid = -1; }
                else ids[id] = c.get();
            }
            checkQuest(c.get());
        }
    }
};

const wchar_t* levelName(CheckLevel l) { return l == CK_ERROR ? L"ERROR  " : l == CK_WARNING ? L"warning" : L"note   "; }

} // namespace

void SchemaCheck(const XDoc& doc, bool clientFile, const NameTable* names, std::vector<CheckItem>& out)
{
    out.clear();
    Checker c{ doc, clientFile, names, out };
    c.run();
    std::stable_sort(out.begin(), out.end(), [](const CheckItem& a, const CheckItem& b) { return a.level < b.level; });
}

std::wstring SchemaCheckText(const std::vector<CheckItem>& items)
{
    int counts[3] = { 0, 0, 0 };
    for (const auto& it : items) counts[it.level]++;
    std::wstring t = fmt(L"%d errors, %d warnings, %d notes\r\n\r\n", counts[0], counts[1], counts[2]);
    if (items.empty()) t += L"Nothing to report.\r\n";
    for (const auto& it : items)
        t += std::wstring(levelName(it.level)) + L"  " + it.where + L"\r\n         " + it.message + L"\r\n";
    return t;
}

//----------------------------------------------------------------------
// compare
//----------------------------------------------------------------------
namespace {

std::wstring shape(const XNode* n, int depth = 0)
{
    std::wstring s = n->name;
    long idx = n->attrInt(L"index");
    if (idx >= 0) s += L"[" + std::to_wstring(idx) + L"]";
    if (!n->children.empty())
    {
        s += L"(";
        for (size_t i = 0; i < n->children.size(); i++) { if (i) s += L" "; s += shape(n->children[i].get(), depth + 1); }
        s += L")";
    }
    return s;
}

} // namespace

std::wstring SchemaCompareText(const XDoc& a, const std::wstring& aName, const XDoc& b, const std::wstring& bName)
{
    std::map<long, const XNode*> qa, qb;
    if (a.root) for (const auto& q : a.root->children) if (q->name == L"Quest") qa[q->attrInt(L"id")] = q.get();
    if (b.root) for (const auto& q : b.root->children) if (q->name == L"Quest") qb[q->attrInt(L"id")] = q.get();

    std::wstring t = L"A = " + aName + L"\r\nB = " + bName + L"\r\n\r\n";
    int onlyA = 0, onlyB = 0, differ = 0, same = 0;
    for (const auto& kv : qa) if (qb.find(kv.first) == qb.end()) { onlyA++; const XNode* ti = kv.second->child(L"Title"); t += fmt(L"only in A : quest %ld  %s\r\n", kv.first, ti ? XOneLine(ti->text, 50).c_str() : L""); }
    for (const auto& kv : qb) if (qa.find(kv.first) == qa.end()) { onlyB++; const XNode* ti = kv.second->child(L"Title"); t += fmt(L"only in B : quest %ld  %s\r\n", kv.first, ti ? XOneLine(ti->text, 50).c_str() : L""); }
    for (const auto& kv : qa)
    {
        auto it = qb.find(kv.first);
        if (it == qb.end()) continue;
        bool diff = false;
        std::wstring detail;
        for (auto s : sectionNames)
        {
            const XNode* sa = kv.second->child(s);
            const XNode* sb = it->second->child(s);
            std::wstring xa = sa ? shape(sa) : L"(missing)";
            std::wstring xb = sb ? shape(sb) : L"(missing)";
            if (xa != xb) { diff = true; detail += std::wstring(L"    ") + s + L"\r\n      A: " + xa + L"\r\n      B: " + xb + L"\r\n"; }
        }
        // quest-level elements present
        std::wstring ea, eb;
        for (const auto& c : kv.second->children) if (!isSection(c->name)) ea += c->name + L" ";
        for (const auto& c : it->second->children) if (!isSection(c->name)) eb += c->name + L" ";
        if (ea != eb) { diff = true; detail += L"    quest-level elements\r\n      A: " + ea + L"\r\n      B: " + eb + L"\r\n"; }
        if (diff) { differ++; t += fmt(L"differs   : quest %ld\r\n", kv.first) + detail; }
        else same++;
    }
    t = fmt(L"%d quests only in A, %d only in B, %d with a different structure, %d with the same structure (text not compared)\r\n\r\n", onlyA, onlyB, differ, same) + t;
    return t;
}
