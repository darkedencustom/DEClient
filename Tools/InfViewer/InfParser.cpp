// InfParser.cpp - see InfParser.h. Layouts come from the client loaders; the
// comment above each schema names the function that was transcribed.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "InfParser.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cwctype>
#include <functional>
#include <stdexcept>

namespace {

//----------------------------------------------------------------------
// small helpers
//----------------------------------------------------------------------
struct InfError : std::runtime_error
{
    size_t at;
    InfError(const std::string& m, size_t a) : std::runtime_error(m), at(a) {}
};

std::wstring fmt(const wchar_t* f, ...)
{
    wchar_t buf[1024];
    va_list ap;
    va_start(ap, f);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, f, ap);
    va_end(ap);
    return buf;
}

std::wstring lower(std::wstring s)
{
    for (auto& ch : s) ch = (wchar_t)towlower(ch);
    return s;
}

// Binary cursor over the whole file. Every read bounds-checks and throws.
struct Cursor
{
    const uint8_t* p;
    size_t         n;
    size_t         pos = 0;

    // What has been read since the last field was emitted, so the field
    // builder can tell "one plain integer / one MString" from composites.
    size_t  mark     = 0;
    int     reads    = 0;
    InfKind lastKind = INF_NONE;
    int     lastSize = 0;

    explicit Cursor(const std::vector<uint8_t>& d) : p(d.data()), n(d.size()) {}

    void need(size_t k) const
    {
        if (pos + k > n) throw InfError("unexpected end of file", pos);
    }
    void note(InfKind k, int size) { reads++; lastKind = k; lastSize = size; }

    uint8_t  u8()  { need(1); note(INF_INT, 1); return p[pos++]; }
    uint16_t u16() { need(2); note(INF_INT, 2); uint16_t v = (uint16_t)(p[pos] | (p[pos + 1] << 8)); pos += 2; return v; }
    uint32_t u32()
    {
        need(4);
        note(INF_INT, 4);
        uint32_t v = (uint32_t)p[pos] | ((uint32_t)p[pos + 1] << 8) | ((uint32_t)p[pos + 2] << 16) | ((uint32_t)p[pos + 3] << 24);
        pos += 4;
        return v;
    }
    int32_t  i32() { return (int32_t)u32(); }

    // A count that must plausibly fit in the file.
    int32_t count()
    {
        size_t  at = pos;
        int32_t c  = i32();
        if (c < 0 || (size_t)c > n) throw InfError("count out of range", at);
        return c;
    }

    // MString::LoadFromFile: 4-byte length + CP949 bytes.
    std::wstring str()
    {
        size_t   at  = pos;
        uint32_t len = u32();
        if (len > 0x10000) throw InfError("string length is absurd - misaligned read", at);
        need(len);
        std::wstring s = InfBytesToWide((const char*)p + pos, len);
        pos += len;
        reads--;                       // the u32 above was part of this string
        note(INF_STR, (int)len + 4);
        return s;
    }
    std::wstring strN(size_t len)
    {
        need(len);
        note(INF_NONE, (int)len);
        std::wstring s = InfBytesToWide((const char*)p + pos, len);
        pos += len;
        return s;
    }
};

// Field builder on a record.
struct Rec
{
    InfRecord& r;
    Cursor&    c;
    Rec(InfRecord& rr, Cursor& cc) : r(rr), c(cc) { c.mark = c.pos; c.reads = 0; }

    // Emit a field covering everything read since the previous field. It is
    // editable only when that was exactly one plain integer or one MString.
    Rec& add(const std::wstring& name, const std::wstring& v)
    {
        InfField f;
        f.name   = name;
        f.value  = v;
        f.offset = c.mark;
        f.length = c.pos - c.mark;
        if (c.reads == 1 && v != L"(none)")
        {
            f.kind = c.lastKind;
            f.size = c.lastSize;
        }
        r.fields.push_back(f);
        c.mark  = c.pos;
        c.reads = 0;
        return *this;
    }
    Rec& add(const std::wstring& name, const wchar_t* v)      { return add(name, std::wstring(v)); }
    // A structural count: shown, never editable by hand.
    Rec& cnt(const std::wstring& name, int v)
    {
        add(name, std::to_wstring(v));
        r.fields.back().kind = INF_NONE;
        return *this;
    }
    Rec& add(const std::wstring& name, long long v)           { return add(name, std::to_wstring(v)); }
    Rec& add(const std::wstring& name, int v)                 { return add(name, std::to_wstring(v)); }
    Rec& add(const std::wstring& name, unsigned v)            { return add(name, std::to_wstring(v)); }
    Rec& flag(const std::wstring& name, bool v)               { return add(name, v ? L"true" : L"false"); }
    Rec& hex(const std::wstring& name, unsigned v, int digits)
    {
        return add(name, fmt(L"0x%0*X (%u)", digits, v, v));
    }
};

InfRecord& newRec(InfDoc& d, const std::wstring& key, size_t off)
{
    d.records.emplace_back();
    InfRecord& r = d.records.back();
    r.key    = key;
    r.offset = off;
    return r;
}

template <class T>
std::wstring joinNums(const std::vector<T>& v)
{
    if (v.empty()) return L"(none)";
    std::wstring s;
    for (size_t i = 0; i < v.size(); i++)
    {
        if (i) s += L", ";
        s += std::to_wstring((long long)v[i]);
    }
    return s;
}

std::vector<uint16_t> readU16List(Cursor& c, int n)
{
    std::vector<uint16_t> v;
    for (int i = 0; i < n; i++) v.push_back(c.u16());
    return v;
}
std::vector<int32_t> readI32List(Cursor& c, int n)
{
    std::vector<int32_t> v;
    for (int i = 0; i < n; i++) v.push_back(c.i32());
    return v;
}

void hdr(InfDoc& d, const std::wstring& name, const std::wstring& v)
{
    d.header.push_back({ name, v });
}

// Generic CTypeTable<T>::LoadFromFile: 4-byte count, then count records; the
// record index is the id the rest of the client uses.
// Mark the single record table of the file (for clone / delete), reading its count.
int beginTable(Cursor& c, InfDoc& d, int kind, int idField = -1, int countSize = 4)
{
    d.tableKind    = kind;
    d.countOffset  = c.pos;
    d.countSize    = countSize;
    d.idFieldIndex = idField;
    int n = countSize == 1 ? c.u8() : c.count();
    hdr(d, L"Count", std::to_wstring(n));
    return n;
}
void endRec(Cursor& c, InfDoc& d, InfRecord& r)
{
    r.length   = c.pos - r.offset;
    d.tableEnd = c.pos;
}

void typeTable(Cursor& c, InfDoc& d, const std::function<void(Cursor&, InfRecord&)>& one)
{
    int n = beginTable(c, d, 1);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, std::to_wstring(i), c.pos);
        one(c, r);
        endRec(c, d, r);
    }
}

// CTypeMap<T>::LoadFromFile: count, then [u32 id][record].
void typeMap(Cursor& c, InfDoc& d, const std::function<void(Cursor&, InfRecord&)>& one)
{
    int n = beginTable(c, d, 2, 0);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, L"", c.pos);
        Rec R(r, c);
        uint32_t id = c.u32();
        r.key = std::to_wstring(id);
        R.add(L"(id)", (unsigned)id);
        one(c, r);
        endRec(c, d, r);
    }
}

// MStringArray = CTypeTable<MString>
void stringArrayInto(Cursor& c, InfDoc& d, const std::wstring& keyPrefix)
{
    int n = c.count();
    hdr(d, keyPrefix + L"Count", std::to_wstring(n));
    for (int i = 0; i < n; i++)
    {
        size_t off = c.pos;
        InfRecord& r = newRec(d, keyPrefix + std::to_wstring(i), off);
        Rec R(r, c);
        std::wstring s = c.str();
        r.summary = s;
        R.add(L"String", s);
        r.length = c.pos - r.offset;
    }
}

// count (countSize bytes) + MStrings, remembered as a growable list on the record.
void addStringList(Rec& R, Cursor& c, const std::wstring& name, int countSize = 4)
{
    InfList L;
    L.name        = name;
    L.countOffset = c.pos;
    L.countSize   = countSize;
    int n = countSize == 1 ? c.u8() : countSize == 2 ? c.u16() : c.count();
    L.count = n;
    R.cnt(name + L".Count", n);
    for (int i = 0; i < n; i++)
    {
        size_t at = c.pos;
        R.add(fmt(L"%s[%d]", name.c_str(), i), c.str());
        L.lastOffset = at;
        L.lastLength = c.pos - at;
    }
    L.insertOffset = c.pos;
    R.r.lists.push_back(L);
}

//----------------------------------------------------------------------
// Schemas
//----------------------------------------------------------------------

// MActionInfoTable::LoadFromFile / MActionInfo::LoadFromFile / ACTION_INFO_NODE
void parseAction(Cursor& c, InfDoc& d)
{
    hdr(d, L"MinResultActionInfo", std::to_wstring(c.i32()));
    hdr(d, L"MaxResultActionInfo", std::to_wstring(c.i32()));
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        r.summary = c.str();
        R.add(L"Name", r.summary);
        R.add(L"Action", c.u8());
        R.add(L"ActionEffectSpriteType", c.u16());
        R.add(L"ActionEffectSpriteTypeFemale", c.u16());
        R.flag(L"UseRepeatFrame", c.u8() != 0);
        for (int s = 0; s < 3; s++)
        {
            std::wstring p = fmt(L"Speed%d.", s);
            R.add(p + L"StartFrame", c.i32());
            R.add(p + L"CastingStartFrame", c.i32());
            R.add(p + L"CastingFrames", c.i32());
            R.add(p + L"RepeatStartFrame", c.i32());
            R.add(p + L"RepeatEndFrame", c.i32());
        }
        for (int s = 0; s < 3; s++)
            R.add(fmt(L"Speed%d.ComboSkillStartFrame[0..3]", s), joinNums(readI32List(c, 4)));
        R.add(L"RepeatLimit", c.u16());
        R.flag(L"CastingEffectToSelf", c.u8() != 0);
        // stored as 4 bytes but only the low word is an ACTIONINFO id; the high word is fill
        unsigned cai = c.u32();
        R.add(L"CastingActionInfo", fmt(L"%u  (0x%08X, low16 = %u)", cai, cai, cai & 0xFFFF));
        R.flag(L"CastingAction", c.u8() != 0);
        R.add(L"CastingTime", c.u16());
        R.add(L"Range", c.u8());
        R.hex(L"TargetFlag", c.u8(), 2);
        R.hex(L"StartFlag", c.u8(), 2);
        R.hex(L"UserTypeFlag", c.u8(), 2);
        R.hex(L"WeaponTypeFlag", c.u16(), 4);
        R.hex(L"CurrentWeaponFlag", c.u8(), 2);
        R.hex(L"OptionFlag", c.u8(), 2);
        unsigned pai = c.u32();
        R.add(L"PlusActionInfo", fmt(L"%u  (0x%08X, low16 = %u)", pai, pai, pai & 0xFFFF));
        R.add(L"PacketType", c.u8());
        R.add(L"Delay", c.u16());
        R.add(L"Value", c.i32());
        R.add(L"SoundID", c.u16());
        R.add(L"SoundMaleID", c.u16());
        R.add(L"SoundFemaleID", c.u16());
        R.add(L"MainNode", c.i32());
        R.add(L"ActionResultID", c.u16());
        R.add(L"ActionResultValue", c.i32());
        R.add(L"EffectStatus", c.u16());
        R.flag(L"Attack", c.u8() != 0);
        bool combo = c.u8() != 0;
        R.flag(L"ComboAttackSkill", combo);
        R.add(L"SelectCreatureAttr", c.u8());
        uint8_t fl = c.u8();
        bool useStep = (fl & 0x2) != 0;
        R.flag(L"UseGrade", (fl & 0x1) != 0);
        R.flag(L"UseActionStep", useStep);
        R.flag(L"AttachSelf", (fl & 0x4) != 0);
        if (useStep) R.add(L"ActionStep[0..4]", joinNums(readU16List(c, 5)));
        if (combo) R.add(L"ComboSkillActionStep[0..3]", joinNums(readU16List(c, 4)));
        R.add(L"Parent", c.u16());
        R.add(L"MasterySkillStep", c.u8());
        R.flag(L"IgnoreFailDelay", c.u8() != 0);
        R.add(L"ActionStepCnt", c.u8());
        int nodes = c.count();
        R.cnt(L"Nodes", nodes);
        for (int i = 0; i < nodes; i++)
        {
            std::wstring p = fmt(L"Node[%d].", i);
            R.add(p + L"EffectGeneratorID", c.u16());
            R.add(p + L"EffectSpriteType", c.u16());
            R.add(p + L"Step", c.u16());
            R.add(p + L"Count", c.u16());
            R.add(p + L"LinkCount", c.u16());
            R.add(p + L"SoundID", c.u16());
            R.add(p + L"DelayNode", c.u8());
            R.add(p + L"ResultTime", c.u8());
        }
    });
}

// SKILLINFO_NODE::LoadFromFile (Skill.inf, index == ACTIONINFO id)
void parseSkill(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        r.summary = c.str();
        R.add(L"Name", r.summary);
        R.add(L"HName", c.str());
        R.add(L"Level", c.i32());
        R.add(L"X", c.i32());
        R.add(L"Y", c.i32());
        R.add(L"SpriteID (icon)", c.u16());
        R.add(L"MP", c.i32());
        R.flag(L"Passive", c.u8() != 0);
        R.flag(L"Active", c.u8() != 0);
        R.add(L"SkillStep", c.u8());
        R.add(L"NextSkill", joinNums(readU16List(c, c.count())));
        R.add(L"DelayGroup", joinNums(readU16List(c, c.count())));
        R.add(L"Description", c.str());
        R.flag(L"HideInSkillTree", c.u8() != 0);
    });
}

// MSkillInfoTable::LoadFromFileServerSkillInfo / SKILLINFO_NODE::LoadFromFileServerSkillInfo
void parseSkillInfo(Cursor& c, InfDoc& d)
{
    int n = beginTable(c, d, 2, 0);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, L"", c.pos);
        Rec R(r, c);
        int32_t id = c.i32();
        r.key = std::to_wstring(id);
        R.add(L"SkillType (ACTIONINFO)", id);
        R.add(L"LearnLevel", c.i32());
        int32_t domain = c.i32();
        R.add(L"DomainType", domain);
        r.summary = c.str();
        R.add(L"Name", r.summary);
        R.add(L"HName", c.str());
        R.add(L"MinDamage", c.i32());
        R.add(L"MaxDamage", c.i32());
        R.add(L"MinDelay", c.i32());
        R.add(L"MaxDelay", c.i32());
        R.add(L"MinDuration", c.i32());
        R.add(L"MaxDuration", c.i32());
        R.add(L"MP", c.i32());
        R.add(L"MinRange", c.i32());
        R.add(L"MaxRange", c.i32());
        R.add(L"MaxExp", c.i32());
        if (domain == 7)   // SKILLDOMAIN_OUSTERS
        {
            R.add(L"SkillPoint", c.i32());
            R.add(L"LevelUpPoint", c.i32());
            R.add(L"SkillTypeList", joinNums(readI32List(c, c.count())));
            R.add(L"Fire", c.i32());
            R.add(L"Water", c.i32());
            R.add(L"Earth", c.i32());
            R.add(L"Wind", c.i32());
            R.add(L"Sum", c.i32());
            R.add(L"Wristlet", c.i32());
            R.add(L"Stone1", c.i32());
            R.add(L"Stone2", c.i32());
            R.add(L"Stone3", c.i32());
            R.add(L"Stone4", c.i32());
            R.add(L"ElementalDomain", c.i32());
            R.flag(L"CanDelete", c.u8() != 0);
        }
        endRec(c, d, r);
    }
}

// CTypeTable<MSkillDomain> with MSkillDomain::LoadFromFile (SkillManager.inf).
void parseSkillManager(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        int n = c.count();
        R.cnt(L"SkillCount", n);
        for (int i = 0; i < n; i++)
        {
            uint16_t id = c.u16();
            uint8_t  st = c.u8();
            R.add(fmt(L"Skill[%d]", i), fmt(L"ACTIONINFO=%u  status=%u", id, st));
        }
        r.summary = fmt(L"%d skills", n);
    });
}

// MSkillManager::LoadFromFileServerDomainInfo -> MSkillDomain::LoadFromFileServerDomainInfo -> ExpInfo
void parseDomainExp(Cursor& c, InfDoc& d)
{
    int n = beginTable(c, d, 1);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, L"", c.pos);
        Rec R(r, c);
        int32_t domain = c.i32();
        R.add(L"Domain", domain);
        int32_t level = c.i32();
        R.add(L"Level", level);
        r.key = fmt(L"%d/%d", domain, level);
        R.add(L"GoalExp", (unsigned)c.u32());
        R.add(L"AccumExp", (unsigned)c.u32());
        r.summary = fmt(L"domain %d level %d", domain, level);
        endRec(c, d, r);
    }
}

// ExperienceTable::LoadFromFile* -> ExpInfo::LoadFromFile
void parseExp(Cursor& c, InfDoc& d)
{
    int n = beginTable(c, d, 2, 0);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, L"", c.pos);
        Rec R(r, c);
        int32_t level = c.i32();
        r.key = std::to_wstring(level);
        R.add(L"Level", level);
        unsigned goal = c.u32();
        R.add(L"GoalExp", goal);
        unsigned accum = c.u32();
        R.add(L"AccumExp", accum);
        r.summary = fmt(L"goal %u  accum %u", goal, accum);
        endRec(c, d, r);
    }
}

// EFFECTSPRITETYPETABLE_INFO::LoadFromFile
void parseEffectSpriteType(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        uint8_t blt = c.u8();
        R.add(L"BltType", blt);
        uint16_t frame = c.u16();
        R.add(L"FrameID", frame);
        uint8_t fl = c.u8();
        R.flag(L"RepeatFrame", ((fl >> 1) & 1) != 0);
        R.flag(L"PairFrameBack", (fl & 1) != 0);
        R.add(L"ActionEffectFrameID", c.u16());
        R.add(L"FemaleEffectSpriteType", c.u16());
        int np = c.u8();
        R.add(L"PairFrameIDList", joinNums(readU16List(c, np)));
        int ncg = c.u8();
        R.cnt(L"ColorGroups", ncg);
        for (int i = 0; i < ncg; i++)
        {
            uint16_t color = c.u16(), est = c.u16();
            R.add(fmt(L"ColorGroup[%d]", i), fmt(L"color=%u -> EffectSpriteType=%u", color, est));
        }
        r.summary = fmt(L"FrameID %u  blt %u", frame, blt);
    });
}

// MActionEffectSpriteTypeTable = CTypeTable<CTypeTable<FRAME_TYPE>>
void parseActionEffectSpriteType(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        int n = c.count();
        auto v = readU16List(c, n);
        R.cnt(L"FrameCount", n);
        R.add(L"FrameID[per action]", joinNums(v));
        r.summary = fmt(L"%d frame ids", n);
    });
}

// EFFECTSTATUS_NODE::LoadFromFile
void parseEffectStatus(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        R.flag(L"UseEffectSprite", c.u8() != 0);
        R.flag(L"AttachGround", c.u8() != 0);
        uint16_t est = c.u16();
        R.add(L"EffectSpriteType", est);
        R.hex(L"EffectColor", c.u16(), 4);
        R.add(L"EffectColorPart (ADDON)", c.u8());
        uint16_t ai = c.u16();
        R.add(L"ActionInfo", ai);
        R.add(L"OriginalActionInfo", c.u16());
        R.add(L"SoundID", c.i32());
        r.summary = fmt(L"EffectSpriteType %u  ActionInfo %u", est, ai);
    });
}

// CREATURESPRITETABLE_INFO::LoadFromFile (19 bytes)
void parseCreatureSprite(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        uint16_t frame = c.u16();
        R.add(L"FrameID", frame);
        R.add(L"SpriteFilePosition", c.i32());
        R.add(L"SpriteShadowFilePosition", c.i32());
        uint16_t first = c.u16(), last = c.u16();
        R.add(L"FirstSpriteID", first);
        R.add(L"LastSpriteID", last);
        R.add(L"FirstShadowSpriteID", c.u16());
        R.add(L"LastShadowSpriteID", c.u16());
        uint8_t ct = c.u8();
        std::wstring who;
        if (ct & 0x10) who += L"PLAYER ";
        if (ct & 0x20) who += L"NPC ";
        if (ct & 0x40) who += L"MONSTER ";
        if (ct & 0x01) who += L"slayer ";
        if (ct & 0x02) who += L"vampire ";
        if (ct & 0x04) who += L"ousters ";
        R.add(L"CreatureType", fmt(L"0x%02X %s", ct, who.c_str()));
        r.summary = fmt(L"FrameID %u  sprites %u..%u", frame, first, last);
    });
}

// CreatureSpriteTypeMapper::LoadFromFile
void parseCreatureSpriteMapper(Cursor& c, InfDoc& d)
{
    int n = beginTable(c, d, 1);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, std::to_wstring(i), c.pos);
        Rec R(r, c);
        int m = c.count();
        R.cnt(L"Count", m);
        for (int k = 0; k < m; k++) R.add(fmt(L"CreatureType[%d]", k), c.u16());
        r.summary = fmt(L"%d creature types", m);
        endRec(c, d, r);
    }
}

// CREATURETABLE_INFO::LoadFromFile (+ ITEM_WEARINFO), action count from GetActionMax()
void parseCreature(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        static const wchar_t* tribes[] = { L"SLAYER", L"VAMPIRE", L"NPC", L"SLAYER_NPC", L"OUSTERS", L"OUSTERS_NPC", L"?", L"OBJECT", L"OBJECT_2", L"OBJECT_3" };
        Rec R(r, c);
        r.summary = c.str();
        R.add(L"Name", r.summary);
        int st = c.count();
        auto spriteTypes = readI32List(c, st);
        R.add(L"SpriteTypes", joinNums(spriteTypes));
        R.flag(L"Male", c.u8() != 0);
        uint8_t tribe = c.u8();
        R.add(L"CreatureTribe", fmt(L"%u (%s)", tribe, tribe < 10 ? tribes[tribe] : L"?"));
        R.add(L"MoveTimes", c.u8());
        R.add(L"MoveRatio", c.u8());
        R.add(L"MoveTimesMotor", c.u8());
        R.add(L"Height", c.i32());
        R.add(L"Width", c.i32());
        R.add(L"DeadHeight", c.i32());
        R.add(L"DeadActionInfo", c.u16());
        R.add(L"ColorSet", c.i32());
        R.flag(L"FlyingCreature", c.u8() != 0);
        R.add(L"FlyingHeight", c.i32());
        R.add(L"HeadCut", c.i32());
        R.add(L"HPBarWidth", c.i32());
        R.add(L"ChangeColorSet", c.u16());
        R.add(L"ShadowCount", c.u16());
        R.add(L"EffectStatus", c.i32());
        R.add(L"Level", c.i32());
        // CREATURETABLE_INFO::GetActionMax()
        int max = 0;
        switch (tribe)
        {
            case 0: case 3: max = 40; break;                                               // ACTION_MAX_SLAYER
            case 1: max = (!spriteTypes.empty() && spriteTypes[0] == 204) ? 22 : 17; break; // vampire; sprite 204 uses the ousters set
            case 2: max = 17; break;                                                       // NPC uses ACTION_MAX_VAMPIRE
            case 7: case 8: case 9: max = 17; break;                                       // DK670 objects (NPC layout)
            case 4: case 5: max = 22; break;                                               // ACTION_MAX_OUSTERS
        }
        R.add(L"ActionMax", max);
        R.add(L"ActionSound[]", joinNums(readU16List(c, max)));
        R.add(L"ActionCount[]", joinNums(readI32List(c, max)));
        bool wear = c.u8() != 0;
        R.flag(L"HasItemWearInfo", wear);
        if (wear)
        {
            static const wchar_t* col[] = { L"Skin", L"Hair", L"Jacket", L"Pants", L"Helmet", L"Weapon", L"Shield", L"Motorcycle" };
            for (int i = 0; i < 8; i++) R.add(fmt(L"Wear.%sColor", col[i]), c.u16());
            static const wchar_t* part[] = { L"Hair", L"Jacket", L"Pants", L"Helmet", L"Weapon", L"Shield", L"Motorcycle" };
            for (int i = 0; i < 7; i++) R.add(fmt(L"Wear.%s", part[i]), c.u8());
        }
        R.flag(L"Fade", c.u8() != 0);
        R.flag(L"FadeShadow", c.u8() != 0);
        R.add(L"EffectSpriteType (body effect)", c.i32());
    });
}

// ITEMCLASS_TABLE = CTypeTable<ITEMTYPE_TABLE = CTypeTable<ITEMTABLE_INFO>>
void parseItemImpl(Cursor& c, InfDoc& d, bool onibla)
{
    int classes = c.count();
    hdr(d, L"ItemClasses", std::to_wstring(classes));
    for (int cls = 0; cls < classes; cls++)
    {
        int types = c.count();
        for (int t = 0; t < types; t++)
        {
            InfRecord& r = newRec(d, fmt(L"%d:%d", cls, t), c.pos);
            Rec R(r, c);
            R.add(L"ItemClass", cls);
            R.add(L"ItemType", t);
            std::wstring en = c.str();
            R.add(L"EName", en);
            std::wstring hn = c.str();
            R.add(L"HName", hn);
            r.summary = hn.empty() ? en : hn;
            R.add(L"Description", c.str());
            R.add(L"TileFrameID", c.u16());
            R.add(L"InventoryFrameID", c.u16());
            R.add(L"GearFrameID", c.u16());
            R.add(L"DropFrameID", c.u16());
            R.add(L"AddonMaleFrameID", c.u16());
            R.add(L"AddonFemaleFrameID", c.u16());
            R.add(L"UseFrameID", c.u16());
            R.add(L"UseSoundID", c.u16());
            R.add(L"TileSoundID", c.u16());
            R.add(L"InventorySoundID", c.u16());
            R.add(L"GearSoundID", c.u16());
            R.add(L"GridWidth", c.u8());
            R.add(L"GridHeight", c.u8());
            R.add(L"Price", c.i32());
            R.add(L"Weight", c.u16());
            for (int v = 1; v <= 7; v++) R.add(fmt(L"Value%d", v), c.i32());
            R.add(L"RequireSTR", c.u8());
            R.add(L"RequireDEX", c.u8());
            R.add(L"RequireINT", c.u8());
            R.add(L"RequireSUM", c.u16());
            R.add(L"RequireLevel", c.u8());
            R.add(L"RequireAdvancementLevel", c.u8());
            R.flag(L"MaleOnly", c.u8() != 0);
            R.flag(L"FemaleOnly", c.u8() != 0);
            R.add(L"UseActionInfo", c.i32());
            R.add(L"SilverMax", c.i32());
            R.add(L"ToHit", c.i32());
            R.add(L"MaxNumber", c.i32());
            R.add(L"CriticalHit", c.i32());
            int nopt = c.u8();
            std::vector<int> opts;
            for (int i = 0; i < nopt; i++) opts.push_back(c.u8());
            R.add(L"DefaultOptionList", joinNums(opts));
            R.add(L"ItemStyle", c.i32());
            R.add(L"ElementalType", c.i32());
            R.add(L"Elemental", c.u16());
            R.add(L"Race", c.u8());
            R.add(L"DescriptionFrameID", c.u16());
            R.hex(L"ItemMoveControl", c.u8(), 2);
            R.hex(L"ItemCanAdvance", c.u8(), 2);
            R.add(L"DropItemNameTag", c.i32());
            if (onibla) R.add(L"NormalItemGrade", c.i32());
        }
    }
}
void parseItem(Cursor& c, InfDoc& d)
{
    // __ONIBLA_ITEM is __ON in this build; fall back to the older layout if
    // the file does not end exactly where that layout says it should.
    size_t start = c.pos;
    try
    {
        parseItemImpl(c, d, true);
        if (c.pos == c.n) return;
    }
    catch (const InfError&) {}
    d.header.clear();
    d.records.clear();
    c.pos = start;
    parseItemImpl(c, d, false);
    d.note += L" Decoded WITHOUT the __ONIBLA_ITEM NormalItemGrade field (the file predates that flag).";
}

// ITEMOPTION_TABLE::LoadFromFile / ITEMOPTION_INFO::LoadFromFile
void parseItemOption(Cursor& c, InfDoc& d)
{
    int parts = c.count();
    hdr(d, L"Parts", std::to_wstring(parts));
    for (int i = 0; i < parts; i++)
    {
        std::wstring en = c.str();
        std::wstring hn = c.str();
        hdr(d, fmt(L"Part[%d]", i), en + L" / " + hn);
    }
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        std::wstring en = c.str();
        R.add(L"EName", en);
        std::wstring hn = c.str();
        R.add(L"Name", hn);
        r.summary = hn.empty() ? en : hn;
        R.add(L"Part", c.i32());
        R.add(L"PlusPoint", c.i32());
        R.add(L"PriceMultiplier", c.i32());
        R.add(L"RequireSTR", c.i32());
        R.add(L"RequireDEX", c.i32());
        R.add(L"RequireINT", c.i32());
        R.add(L"RequireSUM", c.i32());
        R.add(L"RequireLevel", c.i32());
        R.add(L"ColorSet", c.i32());
        R.add(L"UpgradeOptionType", c.i32());
        R.add(L"PreviousOptionType", c.i32());
    });
}

// MNPCTable = CTypeMap<NPC_INFO>
void parseNPC(Cursor& c, InfDoc& d)
{
    typeMap(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        r.summary = c.str();
        R.add(L"Name", r.summary);
        int n = c.count();
        std::vector<uint32_t> ids;
        for (int i = 0; i < n; i++) ids.push_back(c.u32());
        R.add(L"ShopTemplateIDs", joinNums(ids));
        R.add(L"Description", c.str());
        R.add(L"SpriteID", c.i32());
    });
}

// MNPCScriptTable::LoadFromFile / NPC_SCRIPT::LoadFromFile
void parseNPCScript(Cursor& c, InfDoc& d)
{
    typeMap(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        r.summary = c.str();
        R.add(L"OwnerID", r.summary);
        addStringList(R, c, L"Subject");
        addStringList(R, c, L"Content");
    });
}

// MShopTemplateTable = CTypeMap<MShopTemplate>
void parseShopTemplate(Cursor& c, InfDoc& d)
{
    typeMap(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        uint8_t type = c.u8();
        int32_t cls  = c.i32();
        R.add(L"Type", type);
        R.add(L"Class", cls);
        R.add(L"MinType", c.u16());
        R.add(L"MaxType", c.u16());
        R.add(L"MinOption", c.u8());
        R.add(L"MaxOption", c.u8());
        r.summary = fmt(L"type %u  class %d", type, cls);
    });
}

// CZoneTable::LoadFromFile / ZONETABLE_INFO::LoadFromFile / WORLDMAP_INFO::LoadFromFile
void parseZone(Cursor& c, InfDoc& d)
{
    int n = beginTable(c, d, 2, 0);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, L"", c.pos);
        Rec R(r, c);
        uint16_t id = c.u16();
        r.key = std::to_wstring(id);
        R.add(L"ZoneID", id);
        r.summary = c.str();
        R.add(L"Name", r.summary);
        R.hex(L"Property", c.u8(), 2);
        R.add(L"MusicID", c.u16());
        R.add(L"Filename", c.str());
        R.add(L"InfoFilename", c.str());
        R.add(L"TeenFilename", c.str());
        R.add(L"SoundIDs", joinNums(readU16List(c, c.count())));
        R.add(L"Safety", c.u8());
        R.add(L"CreatureColorSet", c.u16());
        R.add(L"MinimapSpriteID", c.u16());
        R.add(L"HolyLand", c.u8());
        R.add(L"ChatMaskByRace", c.u8());
        R.add(L"CannotAttackInSafe", c.u8());
        R.add(L"CannotUseSpecialItem", c.u8());
        R.add(L"CompetenceZone", c.u8());
        R.add(L"PKType", c.u8());
        int32_t wm = c.i32();
        R.add(L"WorldMapType", wm);
        if (wm != 1)   // WORLDMAP_MAX
        {
            R.add(L"WorldMap.PaymentType", c.i32());
            int32_t l = c.i32(), t = c.i32(), rr = c.i32(), b = c.i32();
            R.add(L"WorldMap.Position", fmt(L"left=%d top=%d right=%d bottom=%d", l, t, rr, b));
            uint8_t a1 = c.u8(), a2 = c.u8();
            R.add(L"WorldMap.SlayerLevel", fmt(L"%u..%u", a1, a2));
            a1 = c.u8(); a2 = c.u8();
            R.add(L"WorldMap.VampireLevel", fmt(L"%u..%u", a1, a2));
            a1 = c.u8(); a2 = c.u8();
            R.add(L"WorldMap.OustersLevel", fmt(L"%u..%u", a1, a2));
            R.add(L"WorldMap.MiscDescription", c.str());
        }
        addStringList(R, c, L"MapShape", 1);
        endRec(c, d, r);
    }
}

void parseStringArray(Cursor& c, InfDoc& d)
{
    stringArrayInto(c, d, L"");
    d.tableKind   = 1;
    d.countOffset = 0;
    d.countSize   = 4;
    d.tableEnd    = c.pos;
}

// MonsterNameTable::LoadFromFile: three MStringArrays
void parseMonsterName(Cursor& c, InfDoc& d)
{
    stringArrayInto(c, d, L"first.");
    stringArrayInto(c, d, L"middle.");
    stringArrayInto(c, d, L"last.");
    hdr(d, L"Count", std::to_wstring(d.records.size()));
}

// CTypeTable::LoadFromFile_NickNameString: count, then [u16 index][MString]
void parseNickname(Cursor& c, InfDoc& d)
{
    int n = beginTable(c, d, 2, 0);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, L"", c.pos);
        Rec R(r, c);
        uint16_t idx = c.u16();
        r.key = std::to_wstring(idx);
        R.add(L"(index)", idx);
        r.summary = c.str();
        R.add(L"String", r.summary);
        endRec(c, d, r);
    }
}

void parseSound(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        r.summary = c.str();
        Rec(r, c).add(L"Filename", r.summary);
    });
}
void parseMusic(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        r.summary = c.str();
        Rec(r, c).add(L"Filename", r.summary).add(L"FilenameWav", c.str());
    });
}

// FameInfoTable = CTypeTable<FameInfo>
void parseFameLimit(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        uint8_t dom = c.u8(), lv = c.u8();
        unsigned fame = c.u32();
        Rec(r, c).add(L"DomainType", dom).add(L"Level", lv).add(L"Fame", fame);
        r.summary = fmt(L"domain %u  level %u  fame %u", dom, lv, fame);
    });
}

// RankBonusTable = CTypeTable<RankBonusInfo>
void parseRankBonus(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        R.add(L"Type", c.u16());
        r.summary = c.str();
        R.add(L"Name", r.summary);
        R.add(L"Level", c.u8());
        R.add(L"Race", c.u8());
        R.add(L"Point", c.i32());
        R.add(L"SkillIconID", c.u16());
    });
}

// MGuildInfoMapper::LoadFromFile: count, [u16 guildID][u16 spriteID]
void parseGuildMapper(Cursor& c, InfDoc& d)
{
    int n = beginTable(c, d, 2, 0);
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, L"", c.pos);
        Rec R(r, c);
        uint16_t gid = c.u16();
        r.key = std::to_wstring(gid);
        R.add(L"GuildID", gid);
        uint16_t spr = c.u16();
        R.add(L"SpriteID", spr);
        r.summary = fmt(L"sprite %u", spr);
        endRec(c, d, r);
    }
}

// USINGCOLORSET_NODE::LoadFromFile
void parseUsingColorSet(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        int n = c.count();
        std::vector<int> on;
        for (int i = 0; i < n; i++) if (c.u8()) on.push_back(i);
        Rec(r, c).add(L"Size", n).add(L"EnabledCount", (int)on.size()).add(L"Enabled", joinNums(on));
        r.summary = fmt(L"%u of %d enabled", (unsigned)on.size(), n);
    });
}

// INTERACTIONOBJECTTABLE_INFO::LoadFromFile
void parseInteractionObject(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        uint8_t  type  = c.u8();
        uint16_t frame = c.u16();
        Rec(r, c).add(L"Type", type).add(L"FrameID", frame).add(L"Property", c.i32()).add(L"SoundID", c.i32());
        r.summary = fmt(L"type %u  frame %u", type, frame);
    });
}

// questinfo.inf: DWORD version, then MQuestInfoManager::LoadFromFile (count + records, no id prefix:
// the map key comes from the record's own ID field)
void parseQuestInfo(Cursor& c, InfDoc& d)
{
    hdr(d, L"Version", std::to_wstring(c.u32()));
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        R.add(L"Type", c.u8());
        unsigned id = c.u32();
        r.key = std::to_wstring(id);
        R.add(L"ID", id);
        R.add(L"TimeLimit", (unsigned)c.u32());
        R.add(L"CreatureType", (unsigned)c.u32());
        R.add(L"SpriteType", (unsigned)c.u32());
        R.add(L"Chief", c.u8());
        R.add(L"Goal", (unsigned)c.u32());
        R.add(L"EventQuest", c.u8());
        R.add(L"QuestLevel", c.u8());
        r.summary = c.str();
        R.add(L"Name", r.summary);
    });
    d.tableKind    = 2;   // keyed by the ID field (second field, after Type)
    d.idFieldIndex = 1;
}

// WorldPlayerConfigTable::LoadFromFile / PlayerConfigTable::LoadFromFile / PlayerConfig
void parsePlayer(Cursor& c, InfDoc& d)
{
    hdr(d, L"Version", std::to_wstring(c.i32()));
    int n = c.count();
    hdr(d, L"Worlds", std::to_wstring(n));
    for (int i = 0; i < n; i++)
    {
        size_t  off   = c.pos;
        int32_t world = c.i32();
        InfRecord& r = newRec(d, fmt(L"world %d", world), off);
        Rec R(r, c);
        if (world == -1) { r.summary = L"(empty slot)"; continue; }
        int m = c.count();
        R.cnt(L"Players", m);
        for (int p = 0; p < m; p++)
        {
            uint8_t len = c.u8();
            if (len == 0) { R.add(fmt(L"Player[%d]", p), L"(skipped, empty name)"); continue; }
            std::wstring name = c.strN(len);
            uint8_t  slot   = c.u8();
            int32_t  recent = c.i32();
            R.add(fmt(L"Player[%d]", p), fmt(L"%s  lastSlot=%u  recentCount=%d", name.c_str(), slot, recent));
        }
        r.summary = fmt(L"%d players", m);
    }
}

// MChatManager::LoadFromFile (non-Netmarble path): five curse MStringMaps then the ID map
void parseChat(Cursor& c, InfDoc& d)
{
    static const wchar_t* names[] = { L"CurseEng", L"CurseKor1", L"CurseKor2", L"CurseKor3", L"CurseKor4", L"ID" };
    for (int m = 0; m < 6; m++)
    {
        int n = c.count();
        hdr(d, fmt(L"%s.Count", names[m]), std::to_wstring(n));
        for (int i = 0; i < n; i++)
        {
            size_t off  = c.pos;
            bool   same = c.u8() != 0;
            InfRecord& r = newRec(d, fmt(L"%s.%d", names[m], i), off);
            std::wstring key = c.str();
            std::wstring val = same ? key : c.str();
            Rec(r, c).add(L"Key", key).add(L"Value", val);
            r.summary = key;
        }
    }
}

// MGPS_INFO::LoadFromFile and its five sub-loaders
void parseGps(Cursor& c, InfDoc& d)
{
    {
        int n = c.count();
        hdr(d, L"TitleMonsterInfo", std::to_wstring(n));
        for (int i = 0; i < n; i++)
        {
            InfRecord& r = newRec(d, fmt(L"title.%d", i), c.pos);
            Rec R(r, c);
            R.add(L"ZoneID", c.u16());
            r.summary = c.str();
            R.add(L"ZoneName", r.summary);
            R.add(L"MinMonsterLv", c.u16());
            R.add(L"MaxMonsterLv", c.u16());
        }
    }
    {
        int n = c.count();
        hdr(d, L"ZoneInfo", std::to_wstring(n));
        for (int i = 0; i < n; i++)
        {
            InfRecord& r = newRec(d, fmt(L"zone.%d", i), c.pos);
            r.summary = c.str();
            Rec(r, c).add(L"GpsTitle_ZoneName", r.summary).add(L"ZoneID", c.u16());
        }
    }
    {
        int n = c.count();
        hdr(d, L"NpcInfo", std::to_wstring(n));
        for (int i = 0; i < n; i++)
        {
            InfRecord& r = newRec(d, fmt(L"npc.%d", i), c.pos);
            Rec R(r, c);
            R.add(L"X", c.u16());
            R.add(L"Y", c.u16());
            r.summary = c.str();
            R.add(L"NpcName", r.summary);
            R.add(L"NpcExplanation", c.str());
            R.add(L"SpkIndex", c.u16());
            R.add(L"ZoneID", c.u16());
            R.add(L"MapWidth", c.u16());
            R.add(L"MapHeight", c.u16());
        }
    }
    {
        int n = c.count();
        hdr(d, L"PortalInfo", std::to_wstring(n));
        for (int i = 0; i < n; i++)
        {
            InfRecord& r = newRec(d, fmt(L"portal.%d", i), c.pos);
            Rec R(r, c);
            R.add(L"X", c.u16());
            R.add(L"Y", c.u16());
            r.summary = c.str();
            R.add(L"PortalName", r.summary);
            R.add(L"SpkIndex", c.u16());
            R.add(L"ZoneID", c.u16());
            R.add(L"MapWidth", c.u16());
            R.add(L"MapHeight", c.u16());
        }
    }
    {
        int n = c.count();
        hdr(d, L"MonsterInfo", std::to_wstring(n));
        for (int i = 0; i < n; i++)
        {
            InfRecord& r = newRec(d, fmt(L"monster.%d", i), c.pos);
            Rec R(r, c);
            R.add(L"ZoneID", c.u16());
            r.summary = c.str();
            R.add(L"ZoneName", r.summary);
            R.add(L"MonsterLv", c.str());
            int e = c.u16();
            if (e > 20) throw InfError("gps explanation count > 20", c.pos - 2);
            for (int k = 0; k < e; k++) R.add(fmt(L"Explanation[%d]", k), c.str());
            R.add(L"MinMonsterLv", c.u16());
            R.add(L"MaxMonsterLv", c.u16());
        }
    }
}

// MZone::MZone horn loader
void parseHorn(Cursor& c, InfDoc& d)
{
    int zones = beginTable(c, d, 1);
    for (int z = 0; z < zones; z++)
    {
        InfRecord& r = newRec(d, std::to_wstring(z), c.pos);
        Rec R(r, c);
        int portals = c.count();
        R.cnt(L"Portals", portals);
        for (int p = 0; p < portals; p++)
        {
            std::wstring pf = fmt(L"Portal[%d].", p);
            R.add(pf + L"zone_id", c.i32());
            R.add(pf + L"x", c.i32());
            R.add(pf + L"y", c.i32());
            R.add(pf + L"portal_x", c.i32());
            R.add(pf + L"portal_y", c.i32());
        }
        r.summary = fmt(L"%d portals", portals);
        endRec(c, d, r);
    }
}

// EventGiftInfo::LoadFromFile (VS_UI_GameCommon.cpp)
void parseEventQuest(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        R.add(L"ID", (unsigned)c.u32());
        R.add(L"Active", c.u8());
        R.add(L"Step", (unsigned)c.u32());
        r.summary = c.str();
        R.add(L"Name", r.summary);
    });
}

// C_VS_UI_POWER_JJANG::SetItemList
void parsePowerJjang(Cursor& c, InfDoc& d)
{
    int n = c.u8();
    hdr(d, L"MaxItem", std::to_wstring(n));
    hdr(d, L"AvailablePoint", std::to_wstring(c.u16()));
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, std::to_wstring(i), c.pos);
        uint8_t cls = c.u8(), type = c.u8(), img = c.u8();
        Rec(r, c).add(L"ItemClass", cls).add(L"ItemType", type).add(L"ItemImageNum", img);
        r.summary = fmt(L"class %u  type %u", cls, type);
    }
}

// AppendPatchTable = CTypeTable<AppendPatch>
void parseAppendPatch(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        Rec R(r, c);
        r.summary = c.str();
        R.add(L"OrgFilename", r.summary);
        R.add(L"OrgFilesize", c.i32());
        R.add(L"OrgSpkSize", c.u16());
        R.add(L"FinalFilesize", c.i32());
        R.add(L"FinalSpkSize", c.u16());
        int n = c.count();
        R.cnt(L"Nodes", n);
        for (int i = 0; i < n; i++)
        {
            std::wstring p = fmt(L"Node[%d].", i);
            R.add(p + L"Filename", c.str());
            R.add(p + L"Filesize", c.i32());
            R.add(p + L"SpkSize", c.u16());
            R.add(p + L"WritePosition", c.i32());
            R.add(p + L"AfterFilesize", c.i32());
            R.add(p + L"AfterSpkSize", c.u16());
        }
    });
}

// MHelpManager::LoadFromFile with MCompareHelpNode / MOutputHelpNode
void helpNode(Cursor& c, Rec& R, const std::wstring& prefix, int depth)
{
    if (depth > 64) throw InfError("help node nesting too deep", c.pos);
    uint8_t t = c.u8();
    if (t == 0)
    {
        R.add(prefix, L"(null)");
    }
    else if (t == 1)
    {
        R.add(prefix + L".Compare", c.u16());
        helpNode(c, R, prefix + L".True", depth + 1);
        helpNode(c, R, prefix + L".False", depth + 1);
    }
    else if (t == 2)
    {
        R.add(prefix + L".Output", joinNums(readU16List(c, c.count())));
    }
    else
        throw InfError("unknown help node type", c.pos - 1);
}
void parseSmartHelp(Cursor& c, InfDoc& d)
{
    int n = c.count();
    hdr(d, L"Count", std::to_wstring(n));
    for (int i = 0; i < n; i++)
    {
        InfRecord& r = newRec(d, std::to_wstring(i), c.pos);
        Rec R(r, c);
        helpNode(c, R, L"Node", 0);
        r.summary = r.fields.empty() ? L"" : r.fields.front().name + L" = " + r.fields.front().value;
    }
}

// ClientConfig::LoadFromFile
void parseClientConfig(Cursor& c, InfDoc& d)
{
    InfRecord& r = newRec(d, L"config", 0);
    Rec R(r, c);
    static const wchar_t* ints[] = {
        L"FPS", L"MAX_SCREENSHOT", L"DELAY_PLAYER_DEAD", L"MAX_CLIENT_MOVE", L"MAX_CREATURE_MOVE_BUFFER",
        L"MAX_LIGHT_DRAW", L"MAX_SOUNDPART", L"MAX_UPDATE_ONETIME", L"MAX_UPDATE_ONETIME_COUNT",
        L"DELAY_GLOBAL_SAY", L"DELAY_SYSTEMMESSAGE", L"DELAY_GAMEMESSAGE", L"MAX_DRAWITEMNAME",
        L"MAX_WAIT_PACKET", L"MAX_PROCESS_PACKET", L"MUSIC_THEME", L"MAX_CHATSTRING",
        L"MAX_CHATSTRING_MINUS_1", L"MAX_CHATSTRING_LENGTH", L"MAX_CHATSTRINGLENGTH_PLUS1",
        L"DELAY_CHATSTRING_KEEP", L"DELAY_CHATSTRING_FADE", L"VALUE_CHATSTRING_FADE", L"MIN_CHATSTRING_COLOR256",
        L"MAX_TEXTUREPART_EFFECT", L"MAX_TEXTUREPART_CREATURESHADOW", L"MAX_TEXTUREPART_IMAGEOBJECTSHADOW",
        L"MAX_TEXTUREPART_ADDON_SHADOW", L"MAX_TEXTUREPART_IMAGEOBJECTSPK", L"FONT_HEIGHT", L"FONT_ITEM_HEIGHT",
        L"MAX_HP_BAR_PIXEL", L"POSITION_HP_BAR", L"PERCENTAGE_ITEM_SOMEWHAT_BROKEN", L"PERCENTAGE_ITEM_ALMOST_BROKEN",
        L"FRAME_DRAW_ORIGINAL_SPRITE"
    };
    for (auto name : ints) R.add(name, c.i32());
    R.hex(L"COLOR_OUTLINE_INTERACTIONOBJECT", c.u16(), 4);
    R.hex(L"COLOR_NAME_ITEM", c.u32(), 8);
    R.hex(L"COLOR_NAME_ITEM_OPTION", c.u32(), 8);
    R.hex(L"COLOR_NAME_ITEM_RARE_OPTION", c.u32(), 8);
    R.hex(L"COLOR_OUTLINE_ITEM", c.u16(), 4);
    R.hex(L"COLOR_NAME_VAMPIRE", c.u32(), 8);
    R.hex(L"COLOR_NAME_SLAYER", c.u32(), 8);
    R.hex(L"COLOR_NAME_NPC", c.u32(), 8);
    R.hex(L"COLOR_OUTLINE_NPC", c.u16(), 4);
    R.hex(L"COLOR_OUTLINE_ATTACK_POSSIBLE", c.u16(), 4);
    R.hex(L"COLOR_OUTLINE_ATTACK_IMPOSSIBLE", c.u16(), 4);
    R.add(L"COLOR_HP_BAR_R", c.u8());
    R.add(L"COLOR_HP_BAR_G", c.u8());
    R.add(L"COLOR_HP_BAR_B", c.u8());
    R.add(L"NEW_USER_REGISTERATION_MODE", c.i32());
    R.add(L"URL_HOMEPAGE", c.str());
    R.add(L"URL_HOMEPAGE_NEW_USER", c.str());
    R.add(L"URL_HOMEPAGE_NEW_USER_NETMARBLE", c.str());
    R.add(L"URL_HOMEPAGE_BILING", c.str());
    // READ_CHECK_EOF tail: each field is optional, the loader stops at end of file.
    struct Tail { const wchar_t* name; int size; };
    static const Tail tail[] = {
        { L"COLOR_NAME_GOOD_MORE", 4 }, { L"COLOR_NAME_GOOD", 4 }, { L"COLOR_NAME_NEUTRAL", 4 },
        { L"COLOR_NAME_EVIL", 4 }, { L"COLOR_NAME_EVIL_MORE", 4 }, { L"BLOOD_DROP_HP_PERCENT", 4 },
        { L"BLOOD_DROP_GAP_TIME", 4 }, { L"BLOOD_DROP_RANDOM_TIME", 4 }, { L"MAX_TEXTUREPART_EFFECTSHADOW", 4 },
        { L"MAX_TEXTUREPART_SCREENEFFECT", 4 }, { L"MAX_REQUEST_SERVICE", 4 }, { L"AFTER_TRADE_ITEM_DROP_DELAY", 4 },
        { L"DELAY_PLAYER_RESURRECT_TIMEOUT", 4 }, { L"AFTER_PARTY_KICK_DELAY", 4 }, { L"CLIENT_COMMUNICATION_UDP_PORT", 4 },
        { L"CLIENT_COMMUNICATION_STATUS_DELAY", 4 }, { L"TRADE_ACCEPT_DELAY_TIME", 4 },
        { L"REGEN_AMOUNT_BURROW", 1 }, { L"REGEN_AMOUNT_CASKET", 1 }, { L"REGEN_AMOUNT_VAMPIRE", 1 }, { L"MAX_SOUND_PER_SECOND", 1 },
        { L"REPEAT_TIME", 4 }, { L"LOCK_TIME", 4 }, { L"UniqueItemColorSet", 4 }, { L"QuestItemColorSet", 4 },
        { L"LevelWarItemColorSet (__LEVEL_WAR_RENEWAL)", 4 }, { L"HPModifyListMax", 1 }, { L"HPModifyListTime", 4 },
        { L"TRACE_CHARACTER_LIMIT_TIME", 4 }, { L"DELAY_PLAYER_RESURRECT_TIMEOUT_SIEGEWAR", 4 }, { L"BATTLE_STAND_KEEP_TIME", 4 },
    };
    for (const auto& f : tail)
    {
        if (c.pos + (size_t)f.size > c.n) break;
        if (f.size == 1) R.add(f.name, c.u8());
        else R.add(f.name, (unsigned)c.u32());
    }
    r.summary = L"client configuration";
}

// Waypoint.inf has no loader left in the client; the bytes are a count followed by int triples.
void parseWaypoint(Cursor& c, InfDoc& d)
{
    typeTable(c, d, [](Cursor& c, InfRecord& r) {
        int32_t a = c.i32(), b = c.i32(), e = c.i32();
        Rec(r, c).add(L"Value1 (zone?)", a).add(L"Value2 (x?)", b).add(L"Value3 (y?)", e);
        r.summary = fmt(L"%d, %d, %d", a, b, e);
    });
}

void parseVersion(Cursor& c, InfDoc& d)
{
    InfRecord& r = newRec(d, L"version", 0);
    uint16_t v = c.u16();
    Rec(r, c).add(L"Version", v).hex(L"Hex", v, 4);
    r.summary = std::to_wstring(v);
}
void parseGuildVersion(Cursor& c, InfDoc& d)
{
    InfRecord& r = newRec(d, L"version", 0);
    unsigned v = c.u32();
    Rec(r, c).add(L"GuildVersion", v);
    r.summary = std::to_wstring(v);
}
void parseIpAddr(Cursor& c, InfDoc& d)
{
    InfRecord& r = newRec(d, L"ip", 0);
    uint8_t a = c.u8(), b = c.u8(), e = c.u8(), f = c.u8();
    Rec(r, c).add(L"Bytes in file order", fmt(L"%u.%u.%u.%u", a, b, e, f)).add(L"Reversed", fmt(L"%u.%u.%u.%u", f, e, b, a));
    r.summary = fmt(L"%u.%u.%u.%u", a, b, e, f);
}

// Plain text files (FileDef.inf, GameClient.inf, Interface.inf, ...)
void parseText(Cursor& c, InfDoc& d)
{
    std::wstring all = c.strN(c.n);
    size_t start = 0;
    int    line  = 1;
    while (start <= all.size())
    {
        size_t nl = all.find(L'\n', start);
        std::wstring l = all.substr(start, nl == std::wstring::npos ? std::wstring::npos : nl - start);
        if (!l.empty() && l.back() == L'\r') l.pop_back();
        if (nl == std::wstring::npos && l.empty()) break;
        InfRecord& r = newRec(d, std::to_wstring(line++), start);
        r.summary = l;
        Rec(r, c).add(L"Line", l);
        if (nl == std::wstring::npos) break;
        start = nl + 1;
    }
    hdr(d, L"Lines", std::to_wstring(d.records.size()));
}

bool printableByte(uint8_t b) { return b >= 0x20 && b != 0x7F; }

// Unknown layout: pull out every plausible length-prefixed string.
void parseScan(Cursor& c, InfDoc& d)
{
    size_t i = 0;
    int    found = 0;
    while (i + 4 <= c.n)
    {
        uint32_t len = (uint32_t)c.p[i] | ((uint32_t)c.p[i + 1] << 8) | ((uint32_t)c.p[i + 2] << 16) | ((uint32_t)c.p[i + 3] << 24);
        if (len >= 2 && len <= 512 && i + 4 + len <= c.n)
        {
            bool ok = true;
            for (uint32_t k = 0; k < len && ok; k++)
            {
                uint8_t b = c.p[i + 4 + k];
                if (!printableByte(b) && b != '\r' && b != '\n' && b != '\t') ok = false;
            }
            if (ok)
            {
                InfRecord& r = newRec(d, fmt(L"0x%X", (unsigned)i), i);
                r.summary = InfBytesToWide((const char*)c.p + i + 4, len);
                Rec(r, c).add(L"Length", (unsigned)len).add(L"String", r.summary);
                i += 4 + len;
                found++;
                continue;
            }
        }
        i++;
    }
    c.pos = c.n;
    hdr(d, L"StringsFound", std::to_wstring(found));
}

void parseHex(Cursor& c, InfDoc& d)
{
    for (size_t off = 0; off < c.n; off += 16)
    {
        InfRecord& r = newRec(d, fmt(L"0x%06X", (unsigned)off), off);
        std::wstring hx, asc;
        for (size_t k = 0; k < 16; k++)
        {
            if (off + k < c.n)
            {
                uint8_t b = c.p[off + k];
                hx += fmt(L"%02X ", b);
                asc += (b >= 0x20 && b < 0x7F) ? (wchar_t)b : L'.';
            }
            else
                hx += L"   ";
            if (k == 7) hx += L" ";
        }
        r.summary = hx + L" " + asc;
        Rec(r, c).add(L"Hex", hx).add(L"ASCII", asc);
    }
    c.pos = c.n;
    hdr(d, L"Bytes", std::to_wstring(c.n));
}

//----------------------------------------------------------------------
// registry
//----------------------------------------------------------------------
struct Schema
{
    const wchar_t* id;
    const wchar_t* title;
    const wchar_t* note;
    void (*fn)(Cursor&, InfDoc&);
    const wchar_t* stems;   // "|stem|stem|" matched against the lower-case file stem
};

const Schema g_schemas[] = {
    { L"action", L"Action / skill effect table (Action.inf)",
      L"MActionInfoTable::LoadFromFile. Record index = ACTIONINFO id. Node[] rows hold the EffectSpriteType each step plays.",
      parseAction, L"|action|" },
    { L"skill", L"Skill tree entries (Skill.inf)",
      L"SKILLINFO_NODE::LoadFromFile. Record index = ACTIONINFO id; SpriteID is the SkillIcon.spk frame.",
      parseSkill, L"|skill|" },
    { L"skillinfo", L"Server skill stats (SkillInfo.inf)",
      L"SKILLINFO_NODE::LoadFromFileServerSkillInfo. Keyed by ACTIONINFO id.",
      parseSkillInfo, L"|skillinfo|" },
    { L"skillmanager", L"Skill domains (SkillManager.inf)",
      L"CTypeTable<MSkillDomain>::LoadFromFile layout. The current client no longer opens this file.",
      parseSkillManager, L"|skillmanager|" },
    { L"domainexp", L"Skill domain exp (DomainExp.inf)",
      L"MSkillManager::LoadFromFileServerDomainInfo.", parseDomainExp, L"|domainexp|" },
    { L"exp", L"Experience table (*Exp.inf, StrBalanceInfo.inf)",
      L"ExperienceTable::LoadFromFile* -> ExpInfo. StrBalanceInfo.inf shares the layout but has no loader left.",
      parseExp, L"|strexp|dexexp|intexp|vampireexp|oustersexp|slayerrankexp|vampirerankexp|oustersrankexp|petexp|advancementclassexp|strbalanceinfo|" },
    { L"effectspritetype", L"Effect sprite types (EffectSpriteType.inf)",
      L"EFFECTSPRITETYPETABLE_INFO::LoadFromFile. Record index = EffectSpriteType id; FrameID indexes Effect.spk.",
      parseEffectSpriteType, L"|effectspritetype|" },
    { L"actioneffectspritetype", L"Per-action effect frames (ActionEffectSpriteType.inf)",
      L"MActionEffectSpriteTypeTable: one frame-id list per record.", parseActionEffectSpriteType, L"|actioneffectspritetype|" },
    { L"effectstatus", L"Effect status (EffectStatus.inf)",
      L"EFFECTSTATUS_NODE::LoadFromFile. Record index = EFFECTSTATUS id.", parseEffectStatus, L"|effectstatus|" },
    { L"creaturesprite", L"Creature sprite ranges (CreatureSprite.inf, AddonSprite.inf, CreatureActionSprite.inf)",
      L"CREATURESPRITETABLE_INFO::LoadFromFile, 19 bytes per record. AddonSprite/CreatureActionSprite are no longer loaded but share the layout.",
      parseCreatureSprite, L"|creaturesprite|addonsprite|creatureactionsprite|" },
    { L"creaturespritemapper", L"Sprite type -> creature types (CreatureSpriteMapper.inf)",
      L"CreatureSpriteTypeMapper::LoadFromFile.", parseCreatureSpriteMapper, L"|creaturespritemapper|" },
    { L"creature", L"Creature table (Creature.inf)",
      L"CREATURETABLE_INFO::LoadFromFile. Record index = creature type. Action arrays sized by tribe (Slayer 40 / Vampire 17 / Ousters 22).",
      parseCreature, L"|creature|" },
    { L"item", L"Item table (Item.inf)",
      L"ITEMCLASS_TABLE -> ITEMTYPE_TABLE -> ITEMTABLE_INFO::LoadFromFile. Key = ItemClass:ItemType.",
      parseItem, L"|item|" },
    { L"itemoption", L"Item options (ItemOption.inf)",
      L"ITEMOPTION_TABLE::LoadFromFile.", parseItemOption, L"|itemoption|" },
    { L"npc", L"NPC table (NPC.inf)", L"MNPCTable (CTypeMap<NPC_INFO>). Key = NPC id.", parseNPC, L"|npc|" },
    { L"npcscript", L"NPC scripts (NPCScript.inf)", L"MNPCScriptTable::LoadFromFile. Key = script id.", parseNPCScript, L"|npcscript|" },
    { L"shoptemplate", L"Shop templates (ShopTemplate.inf)", L"MShopTemplateTable (CTypeMap<MShopTemplate>).", parseShopTemplate, L"|shoptemplate|" },
    { L"zone", L"Zone table (Zone.inf)", L"CZoneTable::LoadFromFile. Key = zone id.", parseZone, L"|zone|" },
    { L"strings", L"String array (String.inf, LevelName.inf, SHelpString.inf, Help.inf)",
      L"MStringArray = CTypeTable<MString>.", parseStringArray, L"|string|levelname|shelpstring|help|" },
    { L"monstername", L"Monster name parts (MonsterName.inf)", L"MonsterNameTable: first / middle / last name arrays.", parseMonsterName, L"|monstername|" },
    { L"nickname", L"Nicknames (Nickname.inf)", L"CTypeTable::LoadFromFile_NickNameString: explicit u16 index per string.", parseNickname, L"|nickname|" },
    { L"sound", L"Sound table (Sound.inf)", L"SOUNDTABLE_INFO::LoadFromFile.", parseSound, L"|sound|" },
    { L"music", L"Music table (Music.inf)", L"MUSICTABLE_INFO::LoadFromFile.", parseMusic, L"|music|" },
    { L"famelimit", L"Fame limits (FameLimit.inf)", L"FameInfo::LoadFromFile.", parseFameLimit, L"|famelimit|" },
    { L"rankbonus", L"Rank bonuses (RankBonus.inf)", L"RankBonusInfo::LoadFromFile.", parseRankBonus, L"|rankbonus|" },
    { L"guildmapper", L"Guild sprite mapper (GuildMapper.inf)", L"MGuildInfoMapper::LoadFromFile.", parseGuildMapper, L"|guildmapper|" },
    { L"usingcolorset", L"Colorset usage (UsingColorSet.inf)", L"USINGCOLORSET_NODE::LoadFromFile.", parseUsingColorSet, L"|usingcolorset|" },
    { L"interactionobject", L"Interaction objects (InteractionObject.inf)", L"INTERACTIONOBJECTTABLE_INFO::LoadFromFile.", parseInteractionObject, L"|interactionobject|" },
    { L"questinfo", L"Monster-kill quests (QuestInfo.inf)", L"DWORD version + MQuestInfoManager::LoadFromFile.", parseQuestInfo, L"|questinfo|" },
    { L"player", L"Player config (Player.inf)", L"WorldPlayerConfigTable::LoadFromFile.", parsePlayer, L"|player|" },
    { L"chat", L"Chat filter maps (Chat.inf, Chat2.inf)", L"MChatManager::LoadFromFile, non-Netmarble path (6 MStringMaps).", parseChat, L"|chat|chat2|" },
    { L"gps", L"GPS map info (Gps.inf)", L"MGPS_INFO::LoadFromFile.", parseGps, L"|gps|" },
    { L"horn", L"Horn portals (Horn.inf)", L"MZone::MZone horn loader.", parseHorn, L"|horn|" },
    { L"eventquest", L"Event gifts (EventQuest.inf)", L"EventGiftInfo::LoadFromFile.", parseEventQuest, L"|eventquest|" },
    { L"powerjjang", L"Power-jjang items (PowerjjangItem.inf)", L"C_VS_UI_POWER_JJANG::SetItemList.", parsePowerJjang, L"|powerjjangitem|" },
    { L"appendpatch", L"Append patches (AppendPatch.inf)", L"AppendPatchTable = CTypeTable<AppendPatch>.", parseAppendPatch, L"|appendpatch|" },
    { L"shelp", L"Smart help tree (SHelp.inf)", L"MHelpManager::LoadFromFile (compare/output nodes).", parseSmartHelp, L"|shelp|" },
    { L"clientconfig", L"Client config (ClientConfig.inf)", L"ClientConfig::LoadFromFile.", parseClientConfig, L"|clientconfig|" },
    { L"waypoint", L"Waypoints (Waypoint.inf)", L"No loader left in the client; layout inferred from the bytes (count + int triples).", parseWaypoint, L"|waypoint|" },
    { L"version", L"Version stamp (Version.inf)", L"2-byte version.", parseVersion, L"|version|" },
    { L"guildversion", L"Guild version (GuildVersion.inf)", L"4-byte version.", parseGuildVersion, L"|guildversion|" },
    { L"ipaddr", L"IP address (IpAddr.inf)", L"4 raw bytes.", parseIpAddr, L"|ipaddr|" },
    { L"text", L"Plain text", L"Line-oriented text file.", parseText, L"|filedef|gameclient|gameclient2|updateclient|netmarble|testserver|crashreport|resolution|regentowerposition|selectedchannel|interface|chinainterface|japaninterface|" },
    { L"scan", L"Unknown: string scan", L"Every plausible length-prefixed string in the file, with its offset.", parseScan, L"|" },
    { L"hex", L"Unknown: hex dump", L"Raw bytes, 16 per row.", parseHex, L"|" },
};

const Schema* findSchema(const std::wstring& id)
{
    for (const auto& s : g_schemas) if (id == s.id) return &s;
    return nullptr;
}

bool looksLikeText(const std::vector<uint8_t>& data)
{
    if (data.empty()) return true;
    bool nl = false;
    for (uint8_t b : data)
    {
        if (b == '\n') nl = true;
        else if (b == '\r' || b == '\t') continue;
        else if (b < 0x20) return false;
    }
    return nl || data.size() < 64;
}

} // namespace

//----------------------------------------------------------------------
// public API
//----------------------------------------------------------------------
std::wstring InfBytesToWide(const char* s, size_t n)
{
    if (n == 0) return L"";
    int len = MultiByteToWideChar(949, 0, s, (int)n, nullptr, 0);
    if (len <= 0)
    {
        std::wstring w;
        for (size_t i = 0; i < n; i++) w += (wchar_t)(uint8_t)s[i];
        return w;
    }
    std::wstring w((size_t)len, L'\0');
    MultiByteToWideChar(949, 0, s, (int)n, &w[0], len);
    return w;
}

const std::vector<InfSchemaInfo>& InfSchemaList()
{
    static std::vector<InfSchemaInfo> v;
    if (v.empty())
        for (const auto& s : g_schemas) v.push_back({ s.id, s.title });
    return v;
}

std::wstring InfGuessSchema(const std::wstring& path, const std::vector<uint8_t>& data)
{
    std::wstring name = path;
    size_t slash = name.find_last_of(L"\\/");
    if (slash != std::wstring::npos) name = name.substr(slash + 1);
    name = lower(name);
    size_t dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos) name = name.substr(0, dot);
    // "foo.en" / "foo.kr" language variants share the layout
    dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos && (name.size() - dot == 3)) name = name.substr(0, dot);

    std::wstring needle = L"|" + name + L"|";
    for (const auto& s : g_schemas)
        if (wcsstr(s.stems, needle.c_str())) return s.id;
    if (looksLikeText(data)) return L"text";
    return L"scan";
}

void InfParse(const std::vector<uint8_t>& data, const std::wstring& schemaId, InfDoc& doc)
{
    doc = InfDoc();
    doc.fileSize = data.size();
    const Schema* s = findSchema(schemaId);
    if (!s) s = findSchema(L"scan");
    doc.schemaId    = s->id;
    doc.schemaTitle = s->title;
    doc.note        = s->note;

    Cursor c(data);
    try
    {
        s->fn(c, doc);
        doc.consumed = c.pos;
        if (c.pos < c.n)
            doc.warnings.push_back(fmt(L"%u trailing byte(s) after the last record (stopped at offset 0x%X of 0x%X).",
                                       (unsigned)(c.n - c.pos), (unsigned)c.pos, (unsigned)c.n));
    }
    catch (const InfError& e)
    {
        doc.ok       = false;
        doc.consumed = std::min(e.at, c.n);
        std::wstring what = InfBytesToWide(e.what(), strlen(e.what()));
        doc.warnings.push_back(fmt(L"Decode failed at offset 0x%X (record %u): %s", (unsigned)e.at,
                                   (unsigned)doc.records.size(), what.c_str()));
    }
    catch (const std::exception& e)
    {
        doc.ok = false;
        doc.warnings.push_back(L"Decode failed: " + InfBytesToWide(e.what(), strlen(e.what())));
    }
}

std::wstring InfDocToText(const InfDoc& doc, const std::wstring& sourcePath)
{
    std::wstring t;
    t += L"InfViewer dump\r\n";
    t += L"Source : " + sourcePath + L"\r\n";
    t += L"Schema : " + doc.schemaTitle + L"  [" + doc.schemaId + L"]\r\n";
    t += L"Note   : " + doc.note + L"\r\n";
    t += fmt(L"Size   : %u bytes, decoded %u bytes - %s\r\n", (unsigned)doc.fileSize, (unsigned)doc.consumed,
             doc.complete() ? L"OK, whole file accounted for" : L"INCOMPLETE");
    for (const auto& w : doc.warnings) t += L"Warning: " + w + L"\r\n";
    if (!doc.header.empty())
    {
        t += L"\r\n[Header]\r\n";
        for (const auto& f : doc.header) t += L"  " + f.name + L" = " + f.value + L"\r\n";
    }
    t += fmt(L"\r\n[Records: %u]\r\n", (unsigned)doc.records.size());
    for (const auto& r : doc.records)
    {
        t += fmt(L"\r\n--- #%s  @0x%X  %s\r\n", r.key.c_str(), (unsigned)r.offset, r.summary.c_str());
        for (const auto& f : r.fields)
        {
            std::wstring v = f.value;
            // keep multi-line values readable
            size_t p = 0;
            while ((p = v.find(L'\n', p)) != std::wstring::npos)
            {
                v.replace(p, 1, L"\r\n      ");
                p += 8;
            }
            t += L"    " + f.name + L" = " + v + L"\r\n";
        }
    }
    return t;
}
