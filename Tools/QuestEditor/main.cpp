// QuestEditor - editor for DarkEden quest files.
//
//   The game server reads   data/SimpleGQuest.xml and data/EventGQuest.xml.
//   The client reads its own copies of both from Data\ui\txt\tutorialetc.rpk
//   (a RAR 2.0 pack) for every piece of quest text it shows.
//
//   QuestEditor.exe [file.xml | pack.rpk | pack.rpk:Entry.xml]
//   QuestEditor.exe --list pack.rpk
//   QuestEditor.exe --unpack pack.rpk outdir
//   QuestEditor.exe --pack out.rpk <folder | file...>
//   QuestEditor.exe --check <file.xml | pack.rpk[:Entry.xml]>
//   QuestEditor.exe --compare <fileA> <fileB>
//   QuestEditor.exe --dump <file.xml | pack.rpk[:Entry.xml]> [out.xml] [--client]
//   QuestEditor.exe --roundtrip pack.rpk
//
// Left: the quests. Right: the selected quest as a tree, and the selected
// element's attributes and text underneath (F2 or double-click a line edits
// it). Everything is undoable (Ctrl+Z). Ctrl+S writes the XML file, or the
// whole pack with the edited entry replaced.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>

#include <algorithm>
#include <cwctype>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "QuestSchema.h"
#include "QuestXml.h"
#include "RarPack.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {

enum
{
    IDC_LIST = 100, IDC_TREE, IDC_DETAIL, IDC_FIND, IDC_STATUS, IDC_LBL_FIND, IDC_PROMPT_LIST,   // never IDOK / IDCANCEL (1 / 2)
    IDM_OPEN = 200, IDM_OPEN_RECENT_XML, IDM_OPEN_RECENT_RPK, IDM_RELOAD, IDM_SAVE, IDM_SAVEAS, IDM_EXPORT_XML, IDM_CLIENTSTYLE, IDM_EXIT,
    IDM_UNDO, IDM_REDO, IDM_FIND, IDM_COPYNODE,
    IDM_QUEST_NEW, IDM_QUEST_DUP, IDM_QUEST_DEL, IDM_QUEST_ID, IDM_QUEST_UP, IDM_QUEST_DOWN,
    IDM_ELEM_EDIT, IDM_ELEM_ADDATTR, IDM_ELEM_RENAME, IDM_ELEM_DUP, IDM_ELEM_DEL, IDM_ELEM_UP, IDM_ELEM_DOWN,
    IDM_CHECK, IDM_COMPARE, IDM_UNPACK, IDM_PACK, IDM_NAMES,
    IDM_REFERENCE, IDM_ABOUT,
    IDM_ADD_CHILD_BASE = 1000,     // + element index
    IDM_ADD_AFTER_BASE = 1200,     // + element index
    IDM_REMATTR_BASE   = 1400      // + attribute index
};

enum DocKind { DOC_NONE = 0, DOC_XML, DOC_RPK };
enum DetailMode { DM_NODE = 0, DM_REPORT, DM_TEXT };

struct Doc
{
    DocKind               kind = DOC_NONE;
    std::wstring          path;        // the .xml, or the .rpk
    std::string           entry;       // entry name inside the pack
    std::vector<RarEntry> entries;     // every entry of the pack (rewritten on save)
    std::unique_ptr<XDoc> xml;
    bool                  clientStyle = false;   // raw text for the client's scanner
    bool                  modified = false;
    std::vector<std::unique_ptr<XDoc>> undo, redo;

    std::wstring name() const
    {
        if (kind == DOC_NONE) return L"";
        std::wstring base = path.substr(path.find_last_of(L"\\/") + 1);
        if (kind == DOC_RPK) return std::wstring(entry.begin(), entry.end()) + L" (" + base + L")";
        return base;
    }
    std::wstring spec() const
    {
        if (kind == DOC_RPK) return path + L":" + std::wstring(entry.begin(), entry.end());
        return path;
    }
};

struct DetailLine
{
    int kind = 0;           // 0 none, 1 existing attribute, 2 known missing attribute, 3 text
    size_t attrIndex = 0;   // kind 1
    std::wstring attrName;  // kind 2
    int reportItem = -1;    // DM_REPORT
};

struct App
{
    HWND hwnd = nullptr, list = nullptr, tree = nullptr, detail = nullptr, find = nullptr, status = nullptr, lblFind = nullptr;
    HMENU menu = nullptr, fileMenu = nullptr, elemMenu = nullptr, addChildMenu = nullptr, addAfterMenu = nullptr, remAttrMenu = nullptr;
    HFONT uiFont = nullptr, monoFont = nullptr;
    HACCEL accel = nullptr;
    int scale100 = 100;

    Doc doc;
    NameTable names;
    std::vector<XNode*> quests;    // Quest nodes in file order
    std::vector<int>    shown;     // indexes into quests after filtering
    int    curQuest = -1;          // index into quests
    XNode* selNode  = nullptr;
    std::wstring filter;

    DetailMode mode = DM_NODE;
    std::vector<DetailLine> lines;
    std::vector<CheckItem>  report;
    bool suppressSel = false;
} g;

int px(int v) { return v * g.scale100 / 100; }

std::wstring lower(std::wstring s) { for (auto& ch : s) ch = (wchar_t)towlower(ch); return s; }
std::wstring fileName(const std::wstring& p) { size_t s = p.find_last_of(L"\\/"); return s == std::wstring::npos ? p : p.substr(s + 1); }
std::wstring dirOf(const std::wstring& p) { size_t s = p.find_last_of(L"\\/"); return s == std::wstring::npos ? L"." : p.substr(0, s); }
std::wstring ext(const std::wstring& p) { std::wstring f = lower(fileName(p)); size_t d = f.find_last_of(L'.'); return d == std::wstring::npos ? L"" : f.substr(d); }
std::wstring str2w(const std::string& s) { return XBytesToWide((const uint8_t*)s.data(), s.size(), XENC_CP949); }
std::string  w2str(const std::wstring& w) { std::string o; XWideToBytes(w, XENC_CP949, o); return o; }

std::wstring fmt(const wchar_t* f, ...)
{
    wchar_t buf[2048];
    va_list ap;
    va_start(ap, f);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, f, ap);
    va_end(ap);
    return buf;
}

std::wstring crlf(const std::wstring& s)
{
    std::wstring o;
    o.reserve(s.size() + 16);
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == L'\n' && (i == 0 || s[i - 1] != L'\r')) o += L'\r';
        o += s[i];
    }
    return o;
}

bool readFile(const std::wstring& path, std::vector<uint8_t>& out)
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

bool writeBytes(const std::wstring& path, const void* p, size_t n)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD wrote = 0;
    BOOL ok = n == 0 || WriteFile(h, p, (DWORD)n, &wrote, nullptr);
    CloseHandle(h);
    return ok && wrote == n;
}

bool isDir(const std::wstring& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring exeDir()
{
    wchar_t buf[MAX_PATH * 2];
    GetModuleFileNameW(nullptr, buf, (DWORD)_countof(buf));
    return dirOf(buf);
}

// recent files live under %LOCALAPPDATA%, not next to the exe (which sits in the repo)
std::wstring iniPath()
{
    wchar_t buf[MAX_PATH * 2] = L"";
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, (DWORD)_countof(buf));
    std::wstring dir = (n > 0 && n < _countof(buf)) ? std::wstring(buf) + L"\\DarkEden" : exeDir();
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\QuestEditor.ini";
}
std::wstring iniGet(const wchar_t* key)
{
    wchar_t buf[2048] = L"";
    GetPrivateProfileStringW(L"Recent", key, L"", buf, (DWORD)_countof(buf), iniPath().c_str());
    return buf;
}
void iniSet(const wchar_t* key, const std::wstring& v) { WritePrivateProfileStringW(L"Recent", key, v.c_str(), iniPath().c_str()); }

//----------------------------------------------------------------------
// console output for the command line forms
//----------------------------------------------------------------------
struct Console
{
    HANDLE out = INVALID_HANDLE_VALUE;
    bool   attached = false;
    std::wstring buffer;
    std::wstring toFile;
} con;

void conInit()
{
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        con.attached = true;
        con.out = GetStdHandle(STD_OUTPUT_HANDLE);
    }
}
void conWrite(const std::wstring& s)
{
    if (!con.toFile.empty()) { con.buffer += s; return; }
    if (con.attached && con.out != INVALID_HANDLE_VALUE && con.out != nullptr)
    {
        DWORD n = 0;
        if (WriteConsoleW(con.out, s.c_str(), (DWORD)s.size(), &n, nullptr)) return;
        std::string u8;
        XWideToBytes(s, XENC_UTF8, u8);
        WriteFile(con.out, u8.data(), (DWORD)u8.size(), &n, nullptr);
        return;
    }
    con.buffer += s;
}
void conDone()
{
    if (!con.toFile.empty())
    {
        std::string u8;
        XWideToBytes(con.buffer, XENC_UTF8, u8);
        writeBytes(con.toFile, u8.data(), u8.size());
    }
    else if (!con.attached && !con.buffer.empty())
        MessageBoxW(nullptr, con.buffer.c_str(), L"QuestEditor", MB_OK);
    if (con.attached)
    {
        // the parent prompt has already been printed; end on a fresh line
        conWrite(L"\r\n");
        FreeConsole();
    }
}

//----------------------------------------------------------------------
// value prompt and list picker (modal windows built at runtime)
//----------------------------------------------------------------------
struct Prompt
{
    HWND wnd = nullptr, label = nullptr, edit = nullptr, listbox = nullptr, ok = nullptr, cancel = nullptr;
    bool done = false, accepted = false, multiline = false, isList = false;
};
Prompt* g_prompt = nullptr;

LRESULT CALLBACK promptProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Prompt* p = g_prompt;
    switch (msg)
    {
        case WM_SIZE:
            if (p && p->wnd == hwnd)
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                int pad = px(10), btnW = px(90), btnH = px(26);
                MoveWindow(p->label, pad, pad, rc.right - pad * 2, px(20), TRUE);
                HWND body = p->isList ? p->listbox : p->edit;
                MoveWindow(body, pad, pad + px(24), rc.right - pad * 2, rc.bottom - pad * 3 - px(24) - btnH, TRUE);
                MoveWindow(p->ok, rc.right - pad - btnW * 2 - px(8), rc.bottom - pad - btnH, btnW, btnH, TRUE);
                MoveWindow(p->cancel, rc.right - pad - btnW, rc.bottom - pad - btnH, btnW, btnH, TRUE);
            }
            return 0;
        case WM_COMMAND:
            if (p && LOWORD(wp) == IDOK) { p->accepted = true; p->done = true; return 0; }
            if (p && LOWORD(wp) == IDCANCEL) { p->done = true; return 0; }
            if (p && p->isList && LOWORD(wp) == IDC_PROMPT_LIST && HIWORD(wp) == LBN_DBLCLK) { p->accepted = true; p->done = true; return 0; }
            break;
        case WM_CLOSE:
            if (p) p->done = true;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void registerPromptClass()
{
    static bool registered = false;
    if (registered) return;
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = promptProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"DEQuestPrompt";
    RegisterClassW(&wc);
    registered = true;
}

bool runPrompt(Prompt& p, int w, int h)
{
    HWND owner = g.hwnd;
    RECT orc;
    if (owner) GetWindowRect(owner, &orc); else SystemParametersInfoW(SPI_GETWORKAREA, 0, &orc, 0);
    int x = orc.left + ((orc.right - orc.left) - w) / 2;
    int y = orc.top + ((orc.bottom - orc.top) - h) / 2;
    SetWindowPos(p.wnd, nullptr, x, y, w, h, SWP_NOZORDER);
    if (owner) EnableWindow(owner, FALSE);
    ShowWindow(p.wnd, SW_SHOW);
    SendMessageW(p.wnd, WM_SIZE, 0, 0);
    SetFocus(p.isList ? p.listbox : p.edit);
    MSG m;
    while (!p.done && GetMessageW(&m, nullptr, 0, 0) > 0)
    {
        if (m.message == WM_KEYDOWN && m.wParam == VK_ESCAPE) { p.done = true; break; }
        if (m.message == WM_KEYDOWN && m.wParam == VK_RETURN && (!p.multiline || GetKeyState(VK_CONTROL) < 0))
        {
            p.accepted = true;
            p.done = true;
            break;
        }
        if (IsDialogMessageW(p.wnd, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return p.accepted;
}

// after the caller has read the controls
void endPrompt(Prompt& p)
{
    HWND owner = g.hwnd;
    if (owner) EnableWindow(owner, TRUE);
    DestroyWindow(p.wnd);
    g_prompt = nullptr;
    if (owner) SetForegroundWindow(owner);
}

std::wstring windowText(HWND h)
{
    int len = GetWindowTextLengthW(h);
    std::wstring t((size_t)len + 1, L'\0');
    GetWindowTextW(h, &t[0], len + 1);
    t.resize((size_t)len);
    return t;
}

bool promptValue(const std::wstring& title, const std::wstring& label, std::wstring& value, bool multiline)
{
    registerPromptClass();
    HINSTANCE hi = GetModuleHandleW(nullptr);
    Prompt p;
    p.multiline = multiline;
    g_prompt = &p;
    p.wnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"DEQuestPrompt", title.c_str(), WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 0, 0, 10, 10, g.hwnd, nullptr, hi, nullptr);
    p.label = CreateWindowW(L"STATIC", label.c_str(), WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS, 0, 0, 0, 0, p.wnd, nullptr, hi, nullptr);
    DWORD es = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
    if (multiline) es = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL;
    p.edit   = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", crlf(value).c_str(), es, 0, 0, 0, 0, p.wnd, nullptr, hi, nullptr);
    p.ok     = CreateWindowW(L"BUTTON", multiline ? L"OK (Ctrl+Enter)" : L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, p.wnd, (HMENU)IDOK, hi, nullptr);
    p.cancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, p.wnd, (HMENU)IDCANCEL, hi, nullptr);
    for (HWND h : { p.label, p.ok, p.cancel }) SendMessageW(h, WM_SETFONT, (WPARAM)g.uiFont, TRUE);
    SendMessageW(p.edit, WM_SETFONT, (WPARAM)g.uiFont, TRUE);
    SendMessageW(p.edit, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);
    if (!multiline) PostMessageW(p.edit, EM_SETSEL, 0, (LPARAM)-1);
    bool ok = runPrompt(p, px(640), multiline ? px(400) : px(170));
    if (ok)
    {
        std::wstring t = windowText(p.edit);
        std::wstring o;
        for (wchar_t ch : t) if (ch != L'\r') o += ch;   // the edit control gave us \r\n
        value = o;
    }
    endPrompt(p);
    return ok;
}

// returns the chosen index, or -1
int pickFromList(const std::wstring& title, const std::wstring& label, const std::vector<std::wstring>& items, int initial = 0)
{
    registerPromptClass();
    HINSTANCE hi = GetModuleHandleW(nullptr);
    Prompt p;
    p.isList = true;
    g_prompt = &p;
    p.wnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"DEQuestPrompt", title.c_str(), WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 0, 0, 10, 10, g.hwnd, nullptr, hi, nullptr);
    p.label = CreateWindowW(L"STATIC", label.c_str(), WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS, 0, 0, 0, 0, p.wnd, nullptr, hi, nullptr);
    p.listbox = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, p.wnd, (HMENU)IDC_PROMPT_LIST, hi, nullptr);
    p.ok     = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, p.wnd, (HMENU)IDOK, hi, nullptr);
    p.cancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, p.wnd, (HMENU)IDCANCEL, hi, nullptr);
    for (HWND h : { p.label, p.listbox, p.ok, p.cancel }) SendMessageW(h, WM_SETFONT, (WPARAM)g.uiFont, TRUE);
    for (const auto& s : items) SendMessageW(p.listbox, LB_ADDSTRING, 0, (LPARAM)s.c_str());
    SendMessageW(p.listbox, LB_SETCURSEL, (WPARAM)initial, 0);
    bool ok = runPrompt(p, px(560), px(380));
    int sel = ok ? (int)SendMessageW(p.listbox, LB_GETCURSEL, 0, 0) : LB_ERR;
    endPrompt(p);
    return sel == LB_ERR ? -1 : sel;
}

void showText(const std::wstring& text);

//----------------------------------------------------------------------
// documents
//----------------------------------------------------------------------
bool parseInto(Doc& d, const std::vector<uint8_t>& bytes, std::wstring& err)
{
    auto x = std::make_unique<XDoc>();
    if (!x->parse(bytes, err)) return false;
    if (!x->root || x->root->name != L"QuestList")
        err = L"The root element is <" + (x->root ? x->root->name : std::wstring(L"?")) + L">; a quest file starts with <QuestList>.";
    d.xml = std::move(x);
    return err.empty();
}

// Picks the entry to edit in a pack: the named one, else the only quest list.
// chooser == nullptr means "no UI": take the first <QuestList> entry.
bool chooseEntry(const std::vector<RarEntry>& entries, std::string wanted, bool interactive, std::string& chosen, std::wstring& err)
{
    std::vector<size_t> xmls;
    for (size_t i = 0; i < entries.size(); i++)
    {
        std::string n = entries[i].name;
        std::string l = n;
        for (auto& ch : l) ch = (char)tolower((unsigned char)ch);
        if (!wanted.empty())
        {
            std::string w = wanted;
            for (auto& ch : w) ch = (char)tolower((unsigned char)ch);
            if (l == w) { chosen = n; return true; }
        }
        if (l.size() > 4 && l.compare(l.size() - 4, 4, ".xml") == 0) xmls.push_back(i);
    }
    if (!wanted.empty()) { err = L"The pack has no entry named " + str2w(wanted) + L"."; return false; }
    if (xmls.empty())
    {
        err = L"This pack holds no .xml entry, so it is not the quest pack.\r\n\r\n"
              L"The client's quest files (SimpleGQuest.xml, EventGQuest.xml) are inside\r\n"
              L"Data\\ui\\txt\\tutorialetc.rpk.\r\n\r\nThis pack contains:";
        size_t shown = 0;
        for (const auto& e : entries)
        {
            if (shown++ >= 12) { err += L"\r\n  ..."; break; }
            err += L"\r\n  " + str2w(e.name);
        }
        return false;
    }
    if (interactive && xmls.size() > 1)
    {
        std::vector<std::wstring> items;
        int initial = 0;
        for (size_t k = 0; k < xmls.size(); k++)
        {
            const RarEntry& e = entries[xmls[k]];
            items.push_back(str2w(e.name) + fmt(L"   (%u bytes, %s)", (unsigned)e.data.size(), RarDosTimeText(e.fileTime).c_str()));
            std::string l = e.name;
            for (auto& ch : l) ch = (char)tolower((unsigned char)ch);
            if (l == "simplegquest.xml") initial = (int)k;
        }
        int sel = pickFromList(L"Open from pack", L"Which file inside the pack?", items, initial);
        if (sel < 0) { err = L""; return false; }
        chosen = entries[xmls[(size_t)sel]].name;
        return true;
    }
    // first entry whose root is QuestList
    for (size_t i : xmls)
    {
        XDoc probe;
        std::wstring e2;
        if (probe.parse(entries[i].data, e2) && probe.root && probe.root->name == L"QuestList") { chosen = entries[i].name; return true; }
    }
    chosen = entries[xmls[0]].name;
    return true;
}

// spec: file.xml | pack.rpk | pack.rpk:Entry.xml
bool openSpec(const std::wstring& spec, bool interactive, Doc& d, std::wstring& err)
{
    Doc nd;
    std::wstring path = spec;
    std::string entry;
    size_t colon = spec.find(L".rpk:");
    if (colon != std::wstring::npos) { path = spec.substr(0, colon + 4); entry = w2str(spec.substr(colon + 5)); }

    if (ext(path) == L".rpk")
    {
        nd.kind = DOC_RPK;
        nd.path = path;
        if (!RarReadAll(path, RPK_PASSWORD, nd.entries, err)) return false;
        std::string chosen;
        if (!chooseEntry(nd.entries, entry, interactive, chosen, err)) return false;
        nd.entry = chosen;
        const RarEntry* e = nullptr;
        for (const auto& x : nd.entries) if (x.name == chosen) e = &x;
        if (!parseInto(nd, e->data, err)) { err = str2w(chosen) + L": " + err; return false; }
        nd.clientStyle = true;
    }
    else
    {
        nd.kind = DOC_XML;
        nd.path = path;
        std::vector<uint8_t> bytes;
        if (!readFile(path, bytes)) { err = L"Cannot read " + path; return false; }
        if (!parseInto(nd, bytes, err)) return false;
        nd.clientStyle = false;
    }
    d = std::move(nd);
    return true;
}

bool saveDoc(Doc& d, const std::wstring& to, std::wstring& err)
{
    std::vector<uint8_t> bytes;
    d.xml->write(bytes, d.clientStyle);
    if (d.kind == DOC_RPK)
    {
        bool found = false;
        for (auto& e : d.entries)
            if (e.name == d.entry) { e.data = bytes; e.fileTime = RarDosTimeNow(); found = true; }
        if (!found) { err = L"The entry is no longer in the pack."; return false; }
        if (!RarWriteAll(to, d.entries, err)) return false;
        // prove the client's own reader gets the same bytes back
        std::vector<uint8_t> back;
        if (!RarReadOne(to, RPK_PASSWORD, d.entry, back, err)) { err = L"The pack was written but could not be read back: " + err; return false; }
        if (back != bytes) { err = L"The pack was written but the entry read back differs."; return false; }
    }
    else
    {
        if (!writeBytes(to, bytes.data(), bytes.size())) { err = L"Could not write " + to; return false; }
    }
    d.path = to;
    d.modified = false;
    return true;
}

//----------------------------------------------------------------------
// quest list / tree state
//----------------------------------------------------------------------
std::wstring questText(const XNode* n)
{
    std::wstring s = n->name + L" " + n->text + L" ";
    for (const auto& a : n->attrs) s += a.value + L" ";
    for (const auto& c : n->children) s += questText(c.get());
    return s;
}

void rebuildQuests()
{
    g.quests.clear();
    g.shown.clear();
    if (g.doc.xml && g.doc.xml->root)
        for (const auto& c : g.doc.xml->root->children)
            if (c->name == L"Quest") g.quests.push_back(c.get());
    std::wstring f = lower(XTrim(g.filter));
    for (size_t i = 0; i < g.quests.size(); i++)
    {
        if (f.empty() || lower(questText(g.quests[i])).find(f) != std::wstring::npos) g.shown.push_back((int)i);
    }
    ListView_SetItemCountEx(g.list, (int)g.shown.size(), LVSICF_NOSCROLL);
    InvalidateRect(g.list, nullptr, TRUE);
}

std::wstring questRace(const XNode* q)
{
    const XNode* h = q->child(L"Happen");
    const XNode* r = h ? h->child(L"Race") : nullptr;
    return r ? r->attrValue(L"race") : L"";
}
std::wstring questLevel(const XNode* q)
{
    const XNode* h = q->child(L"Happen");
    const XNode* l = h ? h->child(L"Level") : nullptr;
    if (!l) return L"";
    std::wstring mn = l->attrValue(L"min"), mx = l->attrValue(L"max");
    if (!mn.empty() && !mx.empty()) return mn + L"-" + mx;
    if (!mn.empty()) return mn + L"+";
    if (!mx.empty()) return L"-" + mx;
    return L"";
}

std::wstring nodeLabel(const XNode* n)
{
    std::wstring s = n->name;
    for (const auto& a : n->attrs) s += L" " + a.name + L"=\"" + XOneLine(a.value, 28) + L"\"";
    if (!n->text.empty()) s += L"   " + XOneLine(n->text, 70);
    return s;
}

HTREEITEM addTreeNode(HTREEITEM parent, XNode* n)
{
    TVINSERTSTRUCTW ins = {};
    ins.hParent = parent;
    ins.hInsertAfter = TVI_LAST;
    ins.item.mask = TVIF_TEXT | TVIF_PARAM;
    std::wstring label = nodeLabel(n);
    ins.item.pszText = &label[0];
    ins.item.lParam = (LPARAM)n;
    HTREEITEM h = TreeView_InsertItem(g.tree, &ins);
    for (auto& c : n->children) addTreeNode(h, c.get());
    return h;
}

void expandAll(HTREEITEM h)
{
    for (; h; h = TreeView_GetNextSibling(g.tree, h))
    {
        TreeView_Expand(g.tree, h, TVE_EXPAND);
        expandAll(TreeView_GetChild(g.tree, h));
    }
}

HTREEITEM findTreeItem(HTREEITEM h, const XNode* n)
{
    for (; h; h = TreeView_GetNextSibling(g.tree, h))
    {
        TVITEMW it = {};
        it.mask = TVIF_PARAM;
        it.hItem = h;
        TreeView_GetItem(g.tree, &it);
        if ((XNode*)it.lParam == n) return h;
        HTREEITEM r = findTreeItem(TreeView_GetChild(g.tree, h), n);
        if (r) return r;
    }
    return nullptr;
}

void setStatus(const std::wstring& s) { SendMessageW(g.status, SB_SETTEXTW, 0, (LPARAM)s.c_str()); }

void updateTitle()
{
    std::wstring t = L"QuestEditor";
    if (g.doc.kind != DOC_NONE) t = (g.doc.modified ? L"*" : L"") + g.doc.name() + L" - " + t;
    SetWindowTextW(g.hwnd, t.c_str());
}

void statusSummary(const std::wstring& extra)
{
    if (g.doc.kind == DOC_NONE) { setStatus(L"Open a quest XML or a client .rpk pack (File > Open)."); return; }
    std::wstring s = (g.doc.modified ? L"*" : L"") + g.doc.name();
    s += fmt(L"  |  %u quests", (unsigned)g.quests.size());
    if (g.shown.size() != g.quests.size()) s += fmt(L" (%u shown)", (unsigned)g.shown.size());
    s += g.doc.xml->encoding == XENC_UTF8 ? L"  |  UTF-8" : L"  |  euc-kr (CP949)";
    s += g.doc.clientStyle ? L"  |  writes client-style XML" : L"  |  writes server-style XML";
    if (!extra.empty()) s += L"  |  " + extra;
    setStatus(s);
}

void showText(const std::wstring& text)
{
    g.mode = DM_TEXT;
    g.lines.clear();
    SetWindowTextW(g.detail, crlf(text).c_str());
}

std::wstring sectionOf(const XNode* n)
{
    for (const XNode* p = n; p && p->parent; p = p->parent)
        if (p->name == L"Happen" || p->name == L"Complete" || p->name == L"Fail" || p->name == L"Reward") return p->name;
    return L"";
}

void showNode(XNode* n)
{
    g.mode = DM_NODE;
    g.lines.clear();
    g.selNode = n;
    if (!n)
    {
        std::wstring t;
        if (g.doc.kind == DOC_NONE)
            t = L"No file open.\r\n\r\nFile > Open takes a server quest file (SimpleGQuest.xml, EventGQuest.xml) or the client's pack (tutorialetc.rpk).\r\nHelp > Element reference lists everything a quest may contain.";
        else
            t = L"Select a quest on the left, then an element in the tree above.\r\n\r\nF2 / double-click a line here edits it. Right-click the tree to add, move or remove elements.";
        SetWindowTextW(g.detail, t.c_str());
        return;
    }
    const ElemSpec* spec = SchemaFind(n->name);
    std::wstring t = SchemaNodePath(n) + L"\r\n";
    std::vector<DetailLine> lines;
    lines.push_back({});
    t += L"<" + n->name + L">";
    if (spec) t += L"   " + std::wstring(spec->help);
    else t += L"   (not an element the server or the client knows)";
    t += L"\r\n\r\n";
    lines.push_back({});
    lines.push_back({});

    size_t w = 4;
    for (const auto& a : n->attrs) w = std::max(w, a.name.size());
    if (spec) for (const auto& a : spec->attrs) w = std::max(w, wcslen(a.name));

    const XNode* questList = g.doc.xml ? g.doc.xml->root.get() : nullptr;
    for (size_t i = 0; i < n->attrs.size(); i++)
    {
        const XAttr& a = n->attrs[i];
        std::wstring name = a.name;
        name.resize(w, L' ');
        std::wstring hint = g.names.hint(*n, a.name, a.value, questList);
        t += L" a  " + name + L" = " + XOneLine(a.value) + (hint.empty() ? L"" : L"      " + hint) + L"\r\n";
        DetailLine dl;
        dl.kind = 1;
        dl.attrIndex = i;
        lines.push_back(dl);
    }
    if (spec)
        for (const auto& a : spec->attrs)
        {
            if (n->attr(a.name)) continue;
            std::wstring name = a.name;
            name.resize(w, L' ');
            t += L" +  " + name + L" =            (not set" + (a.required ? L", the server needs it" : L"") + (a.help[0] ? std::wstring(L"; ") + a.help : L"") + L")\r\n";
            DetailLine dl;
            dl.kind = 2;
            dl.attrName = a.name;
            lines.push_back(dl);
        }
    {
        std::wstring name = L"text";
        name.resize(w, L' ');
        std::wstring body = crlf(n->text);
        t += L" t  " + name + L" = ";
        // continuation lines indented under the value
        std::wstring ind(w + 7, L' ');
        for (size_t i = 0; i < body.size(); i++)
        {
            t += body[i];
            if (body[i] == L'\n') t += ind;
        }
        t += L"\r\n";
        DetailLine dl;
        dl.kind = 3;
        lines.push_back(dl);
        for (wchar_t ch : body) if (ch == L'\n') lines.push_back(dl);
    }
    if (spec && spec->hasText)
    {
        std::wstring sec = sectionOf(n);
        if (sec == L"Complete" || sec == L"Fail")
            t += L"\r\n(step text; the client shows it in the journal with %s = done mark and %d = the count where the server reports one)";
    }
    g.lines = lines;
    SetWindowTextW(g.detail, t.c_str());
}

void showQuest(int qi)
{
    g.curQuest = qi;
    g.selNode = nullptr;
    TreeView_DeleteAllItems(g.tree);
    if (qi < 0 || qi >= (int)g.quests.size()) { showNode(nullptr); return; }
    HTREEITEM root = addTreeNode(TVI_ROOT, g.quests[(size_t)qi]);
    expandAll(root);
    g.suppressSel = true;
    TreeView_SelectItem(g.tree, root);
    g.suppressSel = false;
    showNode(g.quests[(size_t)qi]);
}

void selectQuestIndex(int qi)
{
    int row = -1;
    for (size_t i = 0; i < g.shown.size(); i++) if (g.shown[i] == qi) row = (int)i;
    if (row < 0 && qi >= 0)
    {
        // filtered out: drop the filter
        g.filter.clear();
        SetWindowTextW(g.find, L"");
        rebuildQuests();
        for (size_t i = 0; i < g.shown.size(); i++) if (g.shown[i] == qi) row = (int)i;
    }
    g.suppressSel = true;
    ListView_SetItemState(g.list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    if (row >= 0)
    {
        ListView_SetItemState(g.list, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(g.list, row, FALSE);
    }
    g.suppressSel = false;
    showQuest(row >= 0 ? qi : -1);
}

XNode* questOf(XNode* n)
{
    for (XNode* p = n; p; p = p->parent) if (p->name == L"Quest") return p;
    return nullptr;
}

void selectNode(XNode* n)
{
    XNode* q = questOf(n);
    int qi = -1;
    for (size_t i = 0; i < g.quests.size(); i++) if (g.quests[i] == q) qi = (int)i;
    if (qi < 0) return;
    if (g.curQuest != qi) selectQuestIndex(qi);
    HTREEITEM h = findTreeItem(TreeView_GetRoot(g.tree), n);
    if (h) { TreeView_SelectItem(g.tree, h); TreeView_EnsureVisible(g.tree, h); }
    showNode(n);
}

//----------------------------------------------------------------------
// editing (every change snapshots the document for undo)
//----------------------------------------------------------------------
struct NodeRef
{
    long questId = -1;
    std::vector<size_t> path;   // child indexes from the Quest down
};

NodeRef refOf(const XNode* n)
{
    NodeRef r;
    if (!n) return r;
    std::vector<size_t> rev;
    const XNode* p = n;
    while (p && p->name != L"Quest" && p->parent)
    {
        rev.push_back(p->parent->indexOf(p));
        p = p->parent;
    }
    if (p && p->name == L"Quest") r.questId = p->attrInt(L"id");
    r.path.assign(rev.rbegin(), rev.rend());
    return r;
}

XNode* resolve(const NodeRef& r, int questFallback)
{
    XNode* q = nullptr;
    for (XNode* c : g.quests) if (c->attrInt(L"id") == r.questId) { q = c; break; }
    if (!q && questFallback >= 0 && questFallback < (int)g.quests.size()) q = g.quests[(size_t)questFallback];
    if (!q && !g.quests.empty()) q = g.quests[std::min((size_t)std::max(questFallback, 0), g.quests.size() - 1)];
    if (!q) return nullptr;
    XNode* n = q;
    for (size_t idx : r.path)
    {
        if (n->children.empty()) break;
        n = n->children[std::min(idx, n->children.size() - 1)].get();
    }
    return n;
}

void beginEdit()
{
    g.doc.undo.push_back(g.doc.xml->clone());
    if (g.doc.undo.size() > 200) g.doc.undo.erase(g.doc.undo.begin());
    g.doc.redo.clear();
    g.report.clear();
}

void afterEdit(const NodeRef& sel, const std::wstring& what)
{
    g.doc.modified = true;
    int qFallback = g.curQuest;
    rebuildQuests();
    XNode* n = resolve(sel, qFallback);
    g.curQuest = -1;   // the tree's labels are stale: make selectNode rebuild it
    if (n) selectNode(n); else selectQuestIndex(-1);
    updateTitle();
    statusSummary(what + L" - unsaved, Ctrl+S to write");
}

void restoreSnapshot(std::vector<std::unique_ptr<XDoc>>& from, std::vector<std::unique_ptr<XDoc>>& to, const wchar_t* what)
{
    if (from.empty()) { setStatus(fmt(L"Nothing to %s.", what)); return; }
    NodeRef sel = refOf(g.selNode);
    to.push_back(std::move(g.doc.xml));
    g.doc.xml = std::move(from.back());
    from.pop_back();
    g.report.clear();
    afterEdit(sel, what);
}

bool needDoc()
{
    if (g.doc.kind != DOC_NONE) return true;
    setStatus(L"Open a file first.");
    return false;
}

bool needNode()
{
    if (!needDoc()) return false;
    if (g.selNode) return true;
    setStatus(L"Select an element in the tree first.");
    return false;
}

void editAttribute(XNode* n, const std::wstring& name, bool adding)
{
    std::wstring value = n->attrValue(name);
    std::wstring hint = g.names.hint(*n, name, value, g.doc.xml->root.get());
    if (!promptValue(L"<" + n->name + L"> " + name, (adding ? L"New value for " : L"Value for ") + name + (hint.empty() ? L"" : L"   " + hint), value, false)) return;
    NodeRef sel = refOf(n);
    beginEdit();
    n->setAttr(name, value);
    afterEdit(sel, name + L" set");
}

void editText(XNode* n)
{
    std::wstring value = n->text;
    if (!promptValue(L"<" + n->name + L"> text", L"Text of <" + n->name + L"> (Ctrl+Enter to accept, Enter for a new line)", value, true)) return;
    NodeRef sel = refOf(n);
    beginEdit();
    n->text = XTrim(value);
    afterEdit(sel, L"text changed");
}

void editDetailLine(int line)
{
    if (g.mode == DM_REPORT)
    {
        if (line >= 0 && line < (int)g.lines.size() && g.lines[(size_t)line].reportItem >= 0)
        {
            const CheckItem& it = g.report[(size_t)g.lines[(size_t)line].reportItem];
            if (it.node) selectNode(const_cast<XNode*>(it.node));
            else if (it.questId >= 0) for (size_t i = 0; i < g.quests.size(); i++) if (g.quests[i]->attrInt(L"id") == it.questId) selectQuestIndex((int)i);
        }
        return;
    }
    if (g.mode != DM_NODE || !g.selNode) return;
    if (line < 0 || line >= (int)g.lines.size()) return;
    const DetailLine& dl = g.lines[(size_t)line];
    XNode* n = g.selNode;
    if (dl.kind == 1 && dl.attrIndex < n->attrs.size()) editAttribute(n, n->attrs[dl.attrIndex].name, false);
    else if (dl.kind == 2) editAttribute(n, dl.attrName, true);
    else if (dl.kind == 3) editText(n);
    else setStatus(L"Put the caret on an attribute line or the text line, then press F2.");
}

int detailCaretLine()
{
    DWORD start = 0, end = 0;
    SendMessageW(g.detail, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    return (int)SendMessageW(g.detail, EM_LINEFROMCHAR, (WPARAM)start, 0);
}

void editSelected()
{
    if (g.mode == DM_REPORT) { editDetailLine(detailCaretLine()); return; }
    if (!needNode()) return;
    if (GetFocus() == g.detail) { editDetailLine(detailCaretLine()); return; }
    // from the tree: the text when the element has any, else its first attribute
    XNode* n = g.selNode;
    const ElemSpec* spec = SchemaFind(n->name);
    if (!n->text.empty() || (spec && spec->hasText && n->attrs.empty())) editText(n);
    else if (!n->attrs.empty()) editAttribute(n, n->attrs[0].name, false);
    else if (spec && spec->hasText) editText(n);
    else if (spec && !spec->attrs.empty()) editAttribute(n, spec->attrs[0].name, true);
    else setStatus(L"<" + n->name + L"> has nothing to edit; add an attribute from the Element menu.");
}

void addAttribute()
{
    if (!needNode()) return;
    std::wstring name;
    if (!promptValue(L"Add attribute", L"Attribute name", name, false)) return;
    name = XTrim(name);
    if (name.empty()) return;
    for (wchar_t ch : name) if (!(iswalnum(ch) || ch == L'_' || ch == L'-')) { setStatus(L"Attribute names are letters, digits, _ and -."); return; }
    editAttribute(g.selNode, name, true);
}

void removeAttribute(size_t idx)
{
    if (!needNode()) return;
    XNode* n = g.selNode;
    if (idx >= n->attrs.size()) return;
    NodeRef sel = refOf(n);
    std::wstring name = n->attrs[idx].name;
    beginEdit();
    n->attrs.erase(n->attrs.begin() + (ptrdiff_t)idx);
    afterEdit(sel, name + L" removed");
}

void renameElement()
{
    if (!needNode()) return;
    XNode* n = g.selNode;
    if (n->name == L"Quest") { setStatus(L"The <Quest> element keeps its name."); return; }
    std::wstring name = n->name;
    if (!promptValue(L"Rename element", L"Element name (Help > Element reference lists them)", name, false)) return;
    name = XTrim(name);
    if (name.empty() || name == n->name) return;
    for (wchar_t ch : name) if (!(iswalnum(ch) || ch == L'_')) { setStatus(L"Element names are letters, digits and _."); return; }
    NodeRef sel = refOf(n);
    beginEdit();
    n->name = name;
    afterEdit(sel, L"renamed to " + name);
}

void addElement(size_t elemIndex, bool asChild)
{
    if (!needNode()) return;
    const auto& list = SchemaElements();
    if (elemIndex >= list.size()) return;
    XNode* n = g.selNode;
    XNode* parent = asChild ? n : n->parent;
    if (!parent) { setStatus(L"Nothing can go next to the root."); return; }
    if (!asChild && n->name == L"Quest") { setStatus(L"Use the Quest menu to add quests."); return; }
    std::unique_ptr<XNode> e = SchemaNewElement(list[elemIndex].name);
    // give a new step the next free index in its section
    const ElemSpec& spec = list[elemIndex];
    std::wstring sec = asChild ? (n->name == L"Complete" || n->name == L"Fail" ? n->name : sectionOf(n)) : sectionOf(n);
    if ((spec.group == EG_STEP || spec.group == EG_CONDITION || spec.group == EG_TEXT) && (sec == L"Complete" || sec == L"Fail"))
    {
        long mx = 0;
        XNode* s = asChild ? n : parent;
        while (s && s->name != sec) s = s->parent;
        if (s) for (const auto& c : s->children) mx = std::max(mx, c->attrInt(L"index"));
        if (spec.group != EG_TEXT || !e->attr(L"index")) e->setAttr(L"index", std::to_wstring(mx + 1));
        else e->setAttr(L"index", std::to_wstring(std::max(1L, mx)));
    }
    beginEdit();
    size_t at = asChild ? parent->children.size() : parent->indexOf(n) + 1;
    XNode* added = parent->insertChild(at, std::move(e));
    afterEdit(refOf(added), L"<" + std::wstring(spec.name) + L"> added");
}

void duplicateElement()
{
    if (!needNode()) return;
    XNode* n = g.selNode;
    if (n->name == L"Quest") { setStatus(L"Use Quest > Duplicate quest."); return; }
    XNode* parent = n->parent;
    if (!parent) return;
    beginEdit();
    XNode* added = parent->insertChild(parent->indexOf(n) + 1, n->clone());
    afterEdit(refOf(added), L"<" + n->name + L"> duplicated");
}

void deleteElement()
{
    if (!needNode()) return;
    XNode* n = g.selNode;
    if (n->name == L"Quest") { setStatus(L"Use Quest > Delete quest."); return; }
    XNode* parent = n->parent;
    if (!parent) return;
    size_t at = parent->indexOf(n);
    std::wstring name = n->name;
    NodeRef sel = refOf(parent);
    beginEdit();
    parent->removeChild(at);
    afterEdit(sel, L"<" + name + L"> deleted");
}

void moveElement(int delta)
{
    if (!needNode()) return;
    XNode* n = g.selNode;
    XNode* parent = n->parent;
    if (!parent || n->name == L"Quest") return;
    size_t at = parent->indexOf(n);
    if ((delta < 0 && at == 0) || (delta > 0 && at + 1 >= parent->children.size())) return;
    beginEdit();
    std::unique_ptr<XNode> c = parent->removeChild(at);
    XNode* moved = parent->insertChild(at + (delta < 0 ? -1 : 1), std::move(c));
    afterEdit(refOf(moved), L"moved");
}

long nextQuestId()
{
    long mx = 0;
    for (XNode* q : g.quests) mx = std::max(mx, q->attrInt(L"id"));
    return mx + 1;
}

bool questIdFree(long id) { for (XNode* q : g.quests) if (q->attrInt(L"id") == id) return false; return true; }

bool askQuestId(const std::wstring& title, long& id)
{
    std::wstring v = std::to_wstring(id);
    while (true)
    {
        if (!promptValue(title, L"Quest id (a whole number not used in this file)", v, false)) return false;
        v = XTrim(v);
        wchar_t* end = nullptr;
        long n = wcstol(v.c_str(), &end, 10);
        if (v.empty() || !end || *end || n < 0) { MessageBoxW(g.hwnd, L"Enter a whole number.", L"QuestEditor", MB_OK | MB_ICONWARNING); continue; }
        if (!questIdFree(n)) { MessageBoxW(g.hwnd, fmt(L"Quest %ld already exists in this file.", n).c_str(), L"QuestEditor", MB_OK | MB_ICONWARNING); continue; }
        id = n;
        return true;
    }
}

void newQuest(bool duplicate)
{
    if (!needDoc()) return;
    XNode* root = g.doc.xml->root.get();
    XNode* cur = g.curQuest >= 0 && g.curQuest < (int)g.quests.size() ? g.quests[(size_t)g.curQuest] : nullptr;
    if (duplicate && !cur) { setStatus(L"Select the quest to duplicate first."); return; }
    long id = nextQuestId();
    if (!askQuestId(duplicate ? L"Duplicate quest" : L"New quest", id)) return;
    beginEdit();
    std::unique_ptr<XNode> q = duplicate ? cur->clone() : SchemaNewQuest(id);
    q->setAttr(L"id", std::to_wstring(id));
    size_t at = cur ? root->indexOf(cur) + 1 : root->children.size();
    XNode* added = root->insertChild(at, std::move(q));
    afterEdit(refOf(added), fmt(L"quest %ld added", id));
}

void deleteQuest()
{
    if (!needDoc()) return;
    if (g.curQuest < 0 || g.curQuest >= (int)g.quests.size()) { setStatus(L"Select a quest first."); return; }
    XNode* q = g.quests[(size_t)g.curQuest];
    const XNode* t = q->child(L"Title");
    if (MessageBoxW(g.hwnd, fmt(L"Delete quest %s \"%s\"?", q->attrValue(L"id", L"?").c_str(), t ? XOneLine(t->text, 60).c_str() : L"").c_str(), L"QuestEditor", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    XNode* root = g.doc.xml->root.get();
    size_t at = root->indexOf(q);
    int next = g.curQuest;
    beginEdit();
    root->removeChild(at);
    g.doc.modified = true;
    rebuildQuests();
    if (next >= (int)g.quests.size()) next = (int)g.quests.size() - 1;
    selectQuestIndex(next);
    updateTitle();
    statusSummary(L"quest deleted - unsaved, Ctrl+S to write");
}

void changeQuestId()
{
    if (!needDoc()) return;
    if (g.curQuest < 0 || g.curQuest >= (int)g.quests.size()) { setStatus(L"Select a quest first."); return; }
    XNode* q = g.quests[(size_t)g.curQuest];
    long id = q->attrInt(L"id");
    if (!askQuestId(L"Change quest id", id)) return;
    beginEdit();
    q->setAttr(L"id", std::to_wstring(id));
    NodeRef sel;
    sel.questId = id;
    afterEdit(sel, fmt(L"quest id is now %ld", id));
}

void moveQuest(int delta)
{
    if (!needDoc()) return;
    if (g.curQuest < 0 || g.curQuest >= (int)g.quests.size()) return;
    XNode* root = g.doc.xml->root.get();
    XNode* q = g.quests[(size_t)g.curQuest];
    size_t at = root->indexOf(q);
    size_t to = at;
    // skip over non-Quest siblings
    do { if (delta < 0) { if (to == 0) return; to--; } else { if (to + 1 >= root->children.size()) return; to++; } } while (root->children[to]->name != L"Quest");
    beginEdit();
    std::unique_ptr<XNode> c = root->removeChild(at);
    XNode* moved = root->insertChild(to, std::move(c));
    afterEdit(refOf(moved), L"quest moved");
}

//----------------------------------------------------------------------
// files
//----------------------------------------------------------------------
bool saveCurrent(const std::wstring& to);

bool confirmDiscard()
{
    if (!g.doc.modified) return true;
    int r = MessageBoxW(g.hwnd, (L"Save changes to " + g.doc.name() + L"?").c_str(), L"QuestEditor", MB_YESNOCANCEL | MB_ICONQUESTION);
    if (r == IDCANCEL) return false;
    if (r == IDYES) return saveCurrent(g.doc.path);
    return true;
}

void loadNames()
{
    std::vector<std::wstring> roots = { exeDir() };
    if (g.doc.kind != DOC_NONE) roots.push_back(dirOf(g.doc.path));
    g.names.load(roots);
}

void openSpecUI(const std::wstring& spec)
{
    if (!confirmDiscard()) return;
    Doc d;
    std::wstring err;
    if (!openSpec(spec, true, d, err))
    {
        if (!err.empty()) MessageBoxW(g.hwnd, (spec + L"\r\n\r\n" + err).c_str(), L"QuestEditor", MB_OK | MB_ICONERROR);
        return;
    }
    g.doc = std::move(d);
    g.filter.clear();
    SetWindowTextW(g.find, L"");
    if (g.names.empty()) loadNames();
    CheckMenuItem(g.fileMenu, IDM_CLIENTSTYLE, MF_BYCOMMAND | (g.doc.clientStyle ? MF_CHECKED : MF_UNCHECKED));
    rebuildQuests();
    selectQuestIndex(g.quests.empty() ? -1 : 0);
    updateTitle();
    std::wstring extra;
    if (!g.doc.xml->warnings.empty()) extra = fmt(L"%u warnings while reading, see Tools > Check", (unsigned)g.doc.xml->warnings.size());
    statusSummary(extra);
    iniSet(g.doc.kind == DOC_RPK ? L"RPK" : L"XML", g.doc.spec());
}

bool saveCurrent(const std::wstring& to)
{
    if (g.doc.kind == DOC_NONE) return false;
    // refuse to write text the file's code page cannot hold
    std::wstring all = g.doc.xml->writeText(g.doc.clientStyle);
    std::string bytes, bad;
    std::wstring badChars;
    if (!XWideToBytes(all, g.doc.xml->encoding, bytes, &badChars))
    {
        MessageBoxW(g.hwnd, (L"The text contains characters the file's code page (CP949 / euc-kr) cannot store:\r\n\r\n" + badChars +
                             L"\r\n\r\nReplace them, or there is no way for the client to show them.").c_str(), L"QuestEditor", MB_OK | MB_ICONERROR);
        return false;
    }
    std::wstring err;
    if (!saveDoc(g.doc, to, err))
    {
        MessageBoxW(g.hwnd, err.c_str(), L"QuestEditor", MB_OK | MB_ICONERROR);
        return false;
    }
    updateTitle();
    statusSummary(L"saved " + fileName(to) + (g.doc.kind == DOC_RPK ? L" (pack rewritten; put it back into darkeden.dpk for the client)" : L""));
    iniSet(g.doc.kind == DOC_RPK ? L"RPK" : L"XML", g.doc.spec());
    return true;
}

// the folder holding the client's quest pack, found upward from the exe (empty when not in the repo)
std::wstring questPackDir()
{
    std::wstring dir = exeDir();
    for (int up = 0; up < 6 && !dir.empty(); up++)
    {
        std::wstring cand = dir + L"\\ReleaseData\\Data\\ui\\txt";
        if (isDir(cand)) return cand;
        dir = dirOf(dir);
    }
    return L"";
}

void openDialog()
{
    wchar_t file[MAX_PATH * 4] = L"";
    std::wstring initial = g.doc.kind != DOC_NONE ? dirOf(g.doc.path) : questPackDir();
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"Quest files (*.xml;*.rpk)\0*.xml;*.rpk\0Quest XML (*.xml)\0*.xml\0Client packs (*.rpk)\0*.rpk\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = (DWORD)_countof(file);
    ofn.lpstrInitialDir = initial.empty() ? nullptr : initial.c_str();
    ofn.lpstrTitle  = L"Open a quest file: SimpleGQuest.xml / EventGQuest.xml, or the client's tutorialetc.rpk";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn)) openSpecUI(file);
}

void saveAsDialog()
{
    if (!needDoc()) return;
    wchar_t file[MAX_PATH * 4];
    wcscpy_s(file, fileName(g.doc.path).c_str());
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    bool rpk = g.doc.kind == DOC_RPK;
    ofn.lpstrFilter = rpk ? L"Client packs (*.rpk)\0*.rpk\0All files (*.*)\0*.*\0" : L"Quest XML (*.xml)\0*.xml\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = (DWORD)_countof(file);
    ofn.lpstrDefExt = rpk ? L"rpk" : L"xml";
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&ofn)) saveCurrent(file);
}

// write the quest XML as a plain file (from a pack, or a server-style copy of a client file)
void exportXmlDialog()
{
    if (!needDoc()) return;
    wchar_t file[MAX_PATH * 4];
    wcscpy_s(file, g.doc.kind == DOC_RPK ? str2w(g.doc.entry).c_str() : fileName(g.doc.path).c_str());
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"Quest XML (*.xml)\0*.xml\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = (DWORD)_countof(file);
    ofn.lpstrDefExt = L"xml";
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&ofn)) return;
    std::vector<uint8_t> bytes;
    g.doc.xml->write(bytes, g.doc.clientStyle);
    if (writeBytes(file, bytes.data(), bytes.size())) setStatus(std::wstring(L"Exported ") + file);
    else MessageBoxW(g.hwnd, L"Could not write the file.", L"QuestEditor", MB_OK | MB_ICONERROR);
}

//----------------------------------------------------------------------
// tools
//----------------------------------------------------------------------
void showReport(const std::vector<CheckItem>& items)
{
    g.mode = DM_REPORT;
    g.lines.clear();
    std::wstring t = L"CHECK of " + g.doc.name() + (g.doc.clientStyle ? L"  (as the client's copy)" : L"  (as the server's copy)") + L"\r\n";
    g.lines.push_back({});
    int counts[3] = { 0, 0, 0 };
    for (const auto& it : items) counts[it.level]++;
    t += fmt(L"%d errors, %d warnings, %d notes.   Double-click a line to go to the element.\r\n\r\n", counts[0], counts[1], counts[2]);
    g.lines.push_back({});
    g.lines.push_back({});
    if (items.empty()) { t += L"Nothing to report.\r\n"; g.lines.push_back({}); }
    for (size_t i = 0; i < items.size(); i++)
    {
        const CheckItem& it = items[i];
        const wchar_t* lv = it.level == CK_ERROR ? L"ERROR   " : it.level == CK_WARNING ? L"warning " : L"note    ";
        t += lv + it.where + L"\r\n            " + it.message + L"\r\n";
        DetailLine dl;
        dl.reportItem = (int)i;
        g.lines.push_back(dl);
        g.lines.push_back(dl);
    }
    SetWindowTextW(g.detail, t.c_str());
}

void checkCurrent()
{
    if (!needDoc()) return;
    SchemaCheck(*g.doc.xml, g.doc.clientStyle, g.names.empty() ? nullptr : &g.names, g.report);
    showReport(g.report);
    int errors = 0;
    for (const auto& it : g.report) if (it.level == CK_ERROR) errors++;
    statusSummary(fmt(L"check: %d errors, %u items", errors, (unsigned)g.report.size()));
}

void compareDialog()
{
    if (!needDoc()) return;
    wchar_t file[MAX_PATH * 4] = L"";
    std::wstring recent = iniGet(L"Compare");
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"Quest files (*.xml;*.rpk)\0*.xml;*.rpk\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = (DWORD)_countof(file);
    ofn.lpstrTitle  = L"Compare with (the other side's copy)";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetOpenFileNameW(&ofn)) return;
    std::wstring spec = file;
    // inside a pack, prefer the entry with the same name as the open document
    if (ext(spec) == L".rpk" && g.doc.kind == DOC_RPK) spec += L":" + str2w(g.doc.entry);
    else if (ext(spec) == L".rpk") spec += L":" + fileName(g.doc.path);
    Doc other;
    std::wstring err;
    if (!openSpec(spec, true, other, err))
    {
        // the named entry may be absent: let the user choose
        if (ext(spec) == L".rpk" && !openSpec(file, true, other, err))
        {
            if (!err.empty()) MessageBoxW(g.hwnd, err.c_str(), L"QuestEditor", MB_OK | MB_ICONERROR);
            return;
        }
        else if (ext(spec) != L".rpk") { MessageBoxW(g.hwnd, err.c_str(), L"QuestEditor", MB_OK | MB_ICONERROR); return; }
    }
    iniSet(L"Compare", other.spec());
    std::wstring t = L"COMPARE (structure only: element names, index attributes and order; text is not compared)\r\n\r\n";
    t += SchemaCompareText(*g.doc.xml, g.doc.name(), *other.xml, other.name());
    showText(t);
    statusSummary(L"compared with " + other.name());
}

bool browseFolder(const std::wstring& title, std::wstring& out)
{
    // a folder picker via the file dialog: pick any file, take its folder
    wchar_t file[MAX_PATH * 4] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = (DWORD)_countof(file);
    ofn.lpstrTitle  = title.c_str();
    ofn.Flags       = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOVALIDATE;
    wcscpy_s(file, L"(pick any file in the folder)");
    if (!GetOpenFileNameW(&ofn)) return false;
    out = dirOf(file);
    return true;
}

bool unpackTo(const std::wstring& rpk, const std::wstring& dir, std::wstring& report)
{
    std::vector<RarEntry> entries;
    std::wstring err;
    if (!RarReadAll(rpk, RPK_PASSWORD, entries, err)) { report = err; return false; }
    CreateDirectoryW(dir.c_str(), nullptr);
    for (const auto& e : entries)
    {
        std::wstring p = dir + L"\\" + str2w(e.name);
        if (!writeBytes(p, e.data.data(), e.data.size())) { report = L"Could not write " + p; return false; }
        report += fmt(L"%-40s %7u bytes\r\n", str2w(e.name).c_str(), (unsigned)e.data.size());
    }
    report += fmt(L"\r\n%u files written to %s\r\n", (unsigned)entries.size(), dir.c_str());
    return true;
}

bool packFrom(const std::vector<std::wstring>& files, const std::wstring& outRpk, std::wstring& report)
{
    std::vector<RarEntry> entries;
    for (const auto& f : files)
    {
        RarEntry e;
        e.name = w2str(fileName(f));
        if (!readFile(f, e.data)) { report = L"Cannot read " + f; return false; }
        WIN32_FILE_ATTRIBUTE_DATA fa;
        if (GetFileAttributesExW(f.c_str(), GetFileExInfoStandard, &fa))
        {
            FILETIME lt;
            WORD d = 0, t = 0;
            FileTimeToLocalFileTime(&fa.ftLastWriteTime, &lt);
            FileTimeToDosDateTime(&lt, &d, &t);
            e.fileTime = ((uint32_t)d << 16) | t;
        }
        report += fmt(L"%-40s %7u bytes\r\n", fileName(f).c_str(), (unsigned)e.data.size());
        entries.push_back(std::move(e));
    }
    std::wstring err;
    if (!RarWriteAll(outRpk, entries, err)) { report = err; return false; }
    // read back through the client's own reader
    std::vector<RarEntry> back;
    if (!RarReadAll(outRpk, RPK_PASSWORD, back, err)) { report = L"Written, but the client's reader cannot read it back: " + err; return false; }
    if (back.size() != entries.size()) { report = L"Written, but a different number of entries came back."; return false; }
    for (size_t i = 0; i < back.size(); i++)
        if (back[i].data != entries[i].data) { report = L"Written, but " + str2w(entries[i].name) + L" reads back differently."; return false; }
    report += fmt(L"\r\n%u files packed into %s and read back intact.\r\n", (unsigned)entries.size(), outRpk.c_str());
    return true;
}

std::vector<std::wstring> filesInDir(const std::wstring& dir)
{
    std::vector<std::wstring> out;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return out;
    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        out.push_back(dir + L"\\" + fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(out.begin(), out.end(), [](const std::wstring& a, const std::wstring& b) { return lower(a) < lower(b); });
    return out;
}

void unpackDialog()
{
    wchar_t file[MAX_PATH * 4] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"Client packs (*.rpk)\0*.rpk\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = (DWORD)_countof(file);
    ofn.lpstrTitle  = L"Unpack which .rpk?";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetOpenFileNameW(&ofn)) return;
    std::wstring rpk = file;
    std::wstring dir = rpk.substr(0, rpk.size() - 4);
    if (!promptValue(L"Unpack", L"Folder to unpack into", dir, false)) return;
    std::wstring report;
    bool ok = unpackTo(rpk, dir, report);
    showText((ok ? L"UNPACKED " : L"UNPACK FAILED ") + rpk + L"\r\n\r\n" + report);
}

void packDialog()
{
    std::wstring dir;
    if (!browseFolder(L"Pack which folder? (pick any file inside it)", dir)) return;
    std::vector<std::wstring> files = filesInDir(dir);
    if (files.empty()) { MessageBoxW(g.hwnd, L"The folder has no files.", L"QuestEditor", MB_OK | MB_ICONWARNING); return; }
    wchar_t file[MAX_PATH * 4];
    wcscpy_s(file, (fileName(dir) + L".rpk").c_str());
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"Client packs (*.rpk)\0*.rpk\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = (DWORD)_countof(file);
    ofn.lpstrDefExt = L"rpk";
    ofn.lpstrTitle  = L"Write the pack as";
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&ofn)) return;
    std::wstring report;
    bool ok = packFrom(files, file, report);
    showText((ok ? L"PACKED " : L"PACK FAILED ") + std::wstring(file) + L"\r\n\r\n" + report);
}

void namesInfo()
{
    loadNames();
    std::wstring t = L"NAME TABLES (shown next to ids in the element pane)\r\n\r\n";
    if (g.names.sources.empty()) t += L"None found. The editor looks upward from its own folder and from the open file's folder for:\r\n";
    else { t += L"Read:\r\n"; for (const auto& s : g.names.sources) t += L"  " + s + L"\r\n"; t += L"\r\nLooked for:\r\n"; }
    t += L"  Tools\\BinEditor\\names.json      monsters, item classes / types, options (BinEditor's export from the live database)\r\n";
    t += L"  Docs\\Zones.md                   zone ids\r\n";
    t += L"  ReleaseData\\Data\\info\\npc.en.inf  NPC ids (the client's NPC table)\r\n";
    t += fmt(L"\r\n%u monsters, %u item classes, %u zones, %u NPCs known.\r\n", (unsigned)g.names.monsters.size(), (unsigned)g.names.itemClasses.size(), (unsigned)g.names.zones.size(), (unsigned)g.names.npcs.size());
    for (const auto& p : g.names.problems) t += L"problem: " + p + L"\r\n";
    showText(t);
}

void copyNodeText()
{
    std::wstring t = windowText(g.detail);
    if (!OpenClipboard(g.hwnd)) return;
    EmptyClipboard();
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, (t.size() + 1) * sizeof(wchar_t));
    if (h)
    {
        memcpy(GlobalLock(h), t.c_str(), (t.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(h);
        SetClipboardData(CF_UNICODETEXT, h);
    }
    CloseClipboard();
    setStatus(L"Copied to the clipboard.");
}

//----------------------------------------------------------------------
// window
//----------------------------------------------------------------------
HMENU buildElementSubmenu(int base)
{
    HMENU m = CreatePopupMenu();
    const auto& list = SchemaElements();
    for (int gr = 0; gr < EG_GROUPS; gr++)
    {
        HMENU sub = CreatePopupMenu();
        bool any = false;
        for (size_t i = 0; i < list.size(); i++)
        {
            if (list[i].group != gr) continue;
            std::wstring label = std::wstring(list[i].name) + L"\t" + XOneLine(list[i].help, 60);
            AppendMenuW(sub, MF_STRING, (UINT_PTR)(base + (int)i), label.c_str());
            any = true;
        }
        if (any) AppendMenuW(m, MF_POPUP, (UINT_PTR)sub, SchemaGroupTitle((ElemGroup)gr));
        else DestroyMenu(sub);
    }
    return m;
}

void rebuildRemAttrMenu()
{
    while (GetMenuItemCount(g.remAttrMenu) > 0) DeleteMenu(g.remAttrMenu, 0, MF_BYPOSITION);
    if (!g.selNode || g.selNode->attrs.empty()) { AppendMenuW(g.remAttrMenu, MF_STRING | MF_GRAYED, 0, L"(no attributes)"); return; }
    for (size_t i = 0; i < g.selNode->attrs.size() && i < 100; i++)
        AppendMenuW(g.remAttrMenu, MF_STRING, IDM_REMATTR_BASE + (UINT_PTR)i, (g.selNode->attrs[i].name + L" = " + XOneLine(g.selNode->attrs[i].value, 40)).c_str());
}

void buildMenus()
{
    g.menu = CreateMenu();
    g.fileMenu = CreatePopupMenu();
    AppendMenuW(g.fileMenu, MF_STRING, IDM_OPEN, L"&Open XML or .rpk...\tCtrl+O");
    AppendMenuW(g.fileMenu, MF_STRING, IDM_OPEN_RECENT_XML, L"Open last &server XML");
    AppendMenuW(g.fileMenu, MF_STRING, IDM_OPEN_RECENT_RPK, L"Open last &client pack");
    AppendMenuW(g.fileMenu, MF_STRING, IDM_RELOAD, L"&Reload");
    AppendMenuW(g.fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g.fileMenu, MF_STRING, IDM_SAVE, L"&Save\tCtrl+S");
    AppendMenuW(g.fileMenu, MF_STRING, IDM_SAVEAS, L"Save &As...");
    AppendMenuW(g.fileMenu, MF_STRING, IDM_EXPORT_XML, L"&Export quest XML as a plain file...");
    AppendMenuW(g.fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g.fileMenu, MF_STRING, IDM_CLIENTSTYLE, L"Write client-style XML (raw text, no &entity escaping)");
    AppendMenuW(g.fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g.fileMenu, MF_STRING, IDM_EXIT, L"E&xit");
    AppendMenuW(g.menu, MF_POPUP, (UINT_PTR)g.fileMenu, L"&File");

    HMENU edit = CreatePopupMenu();
    AppendMenuW(edit, MF_STRING, IDM_UNDO, L"&Undo\tCtrl+Z");
    AppendMenuW(edit, MF_STRING, IDM_REDO, L"&Redo\tCtrl+Y");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, IDM_FIND, L"&Find quests\tCtrl+F");
    AppendMenuW(edit, MF_STRING, IDM_COPYNODE, L"&Copy the pane below\tCtrl+Shift+C");
    AppendMenuW(g.menu, MF_POPUP, (UINT_PTR)edit, L"&Edit");

    HMENU quest = CreatePopupMenu();
    AppendMenuW(quest, MF_STRING, IDM_QUEST_NEW, L"&New quest...\tCtrl+N");
    AppendMenuW(quest, MF_STRING, IDM_QUEST_DUP, L"&Duplicate quest...\tCtrl+D");
    AppendMenuW(quest, MF_STRING, IDM_QUEST_ID, L"Change quest &id...");
    AppendMenuW(quest, MF_STRING, IDM_QUEST_DEL, L"De&lete quest\tCtrl+Delete");
    AppendMenuW(quest, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(quest, MF_STRING, IDM_QUEST_UP, L"Move quest &up in the file");
    AppendMenuW(quest, MF_STRING, IDM_QUEST_DOWN, L"Move quest d&own in the file");
    AppendMenuW(g.menu, MF_POPUP, (UINT_PTR)quest, L"&Quest");

    g.elemMenu = CreatePopupMenu();
    g.addChildMenu = buildElementSubmenu(IDM_ADD_CHILD_BASE);
    g.addAfterMenu = buildElementSubmenu(IDM_ADD_AFTER_BASE);
    g.remAttrMenu = CreatePopupMenu();
    AppendMenuW(g.elemMenu, MF_STRING, IDM_ELEM_EDIT, L"&Edit value\tF2");
    AppendMenuW(g.elemMenu, MF_STRING, IDM_ELEM_ADDATTR, L"Add &attribute...");
    AppendMenuW(g.elemMenu, MF_POPUP, (UINT_PTR)g.remAttrMenu, L"&Remove attribute");
    AppendMenuW(g.elemMenu, MF_STRING, IDM_ELEM_RENAME, L"Re&name element...");
    AppendMenuW(g.elemMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g.elemMenu, MF_POPUP, (UINT_PTR)g.addChildMenu, L"Add &child element");
    AppendMenuW(g.elemMenu, MF_POPUP, (UINT_PTR)g.addAfterMenu, L"&Insert element after this one");
    AppendMenuW(g.elemMenu, MF_STRING, IDM_ELEM_DUP, L"&Duplicate element\tCtrl+Shift+D");
    AppendMenuW(g.elemMenu, MF_STRING, IDM_ELEM_DEL, L"De&lete element\tDelete");
    AppendMenuW(g.elemMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(g.elemMenu, MF_STRING, IDM_ELEM_UP, L"Move &up\tCtrl+Up");
    AppendMenuW(g.elemMenu, MF_STRING, IDM_ELEM_DOWN, L"Move d&own\tCtrl+Down");
    AppendMenuW(g.menu, MF_POPUP, (UINT_PTR)g.elemMenu, L"E&lement");

    HMENU tools = CreatePopupMenu();
    AppendMenuW(tools, MF_STRING, IDM_CHECK, L"&Check this file\tCtrl+K");
    AppendMenuW(tools, MF_STRING, IDM_COMPARE, L"Co&mpare structure with the other side...");
    AppendMenuW(tools, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(tools, MF_STRING, IDM_UNPACK, L"&Unpack an .rpk to a folder...");
    AppendMenuW(tools, MF_STRING, IDM_PACK, L"&Pack a folder into an .rpk...");
    AppendMenuW(tools, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(tools, MF_STRING, IDM_NAMES, L"&Name tables (reload)");
    AppendMenuW(g.menu, MF_POPUP, (UINT_PTR)tools, L"&Tools");

    HMENU help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, IDM_REFERENCE, L"&Element reference\tF1");
    AppendMenuW(help, MF_STRING, IDM_ABOUT, L"&About");
    AppendMenuW(g.menu, MF_POPUP, (UINT_PTR)help, L"&Help");

    SetMenu(g.hwnd, g.menu);

    static ACCEL acc[] = {
        { FCONTROL | FVIRTKEY, 'O', IDM_OPEN }, { FCONTROL | FVIRTKEY, 'S', IDM_SAVE }, { FCONTROL | FVIRTKEY, 'Z', IDM_UNDO },
        { FCONTROL | FVIRTKEY, 'Y', IDM_REDO }, { FCONTROL | FVIRTKEY, 'F', IDM_FIND }, { FCONTROL | FSHIFT | FVIRTKEY, 'C', IDM_COPYNODE },
        { FCONTROL | FVIRTKEY, 'N', IDM_QUEST_NEW }, { FCONTROL | FVIRTKEY, 'D', IDM_QUEST_DUP }, { FCONTROL | FVIRTKEY, VK_DELETE, IDM_QUEST_DEL },
        { FVIRTKEY, VK_F2, IDM_ELEM_EDIT }, { FCONTROL | FSHIFT | FVIRTKEY, 'D', IDM_ELEM_DUP }, { FCONTROL | FVIRTKEY, VK_UP, IDM_ELEM_UP },
        { FCONTROL | FVIRTKEY, VK_DOWN, IDM_ELEM_DOWN }, { FCONTROL | FVIRTKEY, 'K', IDM_CHECK }, { FVIRTKEY, VK_F1, IDM_REFERENCE },
    };
    g.accel = CreateAcceleratorTableW(acc, (int)_countof(acc));
}

void layout()
{
    RECT rc;
    GetClientRect(g.hwnd, &rc);
    SendMessageW(g.status, WM_SIZE, 0, 0);
    RECT src;
    GetWindowRect(g.status, &src);
    int statusH = src.bottom - src.top;
    int w = rc.right, h = rc.bottom - statusH;
    int pad = px(6);
    int leftW = std::max(px(300), w * 38 / 100);
    int findH = px(24);
    MoveWindow(g.lblFind, pad, pad + px(4), px(36), findH, TRUE);
    MoveWindow(g.find, pad + px(38), pad, leftW - px(38), findH, TRUE);
    MoveWindow(g.list, pad, pad * 2 + findH, leftW, h - pad * 3 - findH, TRUE);
    // the title column takes whatever the fixed columns leave
    int fixed = 0;
    for (int c : { 0, 2, 3, 4 }) fixed += ListView_GetColumnWidth(g.list, c);
    ListView_SetColumnWidth(g.list, 1, std::max(px(120), leftW - fixed - GetSystemMetrics(SM_CXVSCROLL) - px(6)));
    int rx = pad * 2 + leftW;
    int rw = w - rx - pad;
    int treeH = (h - pad * 3) * 45 / 100;
    MoveWindow(g.tree, rx, pad, rw, treeH, TRUE);
    MoveWindow(g.detail, rx, pad * 2 + treeH, rw, h - pad * 3 - treeH, TRUE);
}

LRESULT CALLBACK detailProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR)
{
    switch (msg)
    {
        case WM_LBUTTONDBLCLK:
        {
            LRESULT r = DefSubclassProc(hwnd, msg, wp, lp);
            int line = (int)SendMessageW(hwnd, EM_LINEFROMCHAR, (WPARAM)SendMessageW(hwnd, EM_CHARFROMPOS, 0, lp) , 0);
            // EM_CHARFROMPOS returns the character in the low word and the line in the high word
            LRESULT cp = SendMessageW(hwnd, EM_CHARFROMPOS, 0, lp);
            line = HIWORD(cp);
            editDetailLine(line);
            return r;
        }
        case WM_KEYDOWN:
            if (wp == VK_F2) { editDetailLine(detailCaretLine()); return 0; }
            if (wp == VK_RETURN && g.mode == DM_REPORT) { editDetailLine(detailCaretLine()); return 0; }
            break;
        case WM_CHAR:
            if (wp == 1) { SendMessageW(hwnd, EM_SETSEL, 0, -1); return 0; }   // Ctrl+A
            break;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

void onCommand(int id)
{
    if (id >= IDM_ADD_CHILD_BASE && id < IDM_ADD_AFTER_BASE) { addElement((size_t)(id - IDM_ADD_CHILD_BASE), true); return; }
    if (id >= IDM_ADD_AFTER_BASE && id < IDM_REMATTR_BASE) { addElement((size_t)(id - IDM_ADD_AFTER_BASE), false); return; }
    if (id >= IDM_REMATTR_BASE && id < IDM_REMATTR_BASE + 200) { removeAttribute((size_t)(id - IDM_REMATTR_BASE)); return; }
    switch (id)
    {
        case IDM_OPEN: openDialog(); break;
        case IDM_OPEN_RECENT_XML: { std::wstring s = iniGet(L"XML"); if (s.empty()) setStatus(L"No server XML has been opened yet."); else openSpecUI(s); break; }
        case IDM_OPEN_RECENT_RPK: { std::wstring s = iniGet(L"RPK"); if (s.empty()) setStatus(L"No client pack has been opened yet."); else openSpecUI(s); break; }
        case IDM_RELOAD: if (g.doc.kind != DOC_NONE) openSpecUI(g.doc.spec()); break;
        case IDM_SAVE: if (needDoc()) saveCurrent(g.doc.path); break;
        case IDM_SAVEAS: saveAsDialog(); break;
        case IDM_EXPORT_XML: exportXmlDialog(); break;
        case IDM_CLIENTSTYLE:
            if (needDoc())
            {
                g.doc.clientStyle = !g.doc.clientStyle;
                CheckMenuItem(g.fileMenu, IDM_CLIENTSTYLE, MF_BYCOMMAND | (g.doc.clientStyle ? MF_CHECKED : MF_UNCHECKED));
                g.doc.modified = true;
                updateTitle();
                statusSummary(L"output style changed");
            }
            break;
        case IDM_EXIT: PostMessageW(g.hwnd, WM_CLOSE, 0, 0); break;
        case IDM_UNDO: if (needDoc()) restoreSnapshot(g.doc.undo, g.doc.redo, L"undo"); break;
        case IDM_REDO: if (needDoc()) restoreSnapshot(g.doc.redo, g.doc.undo, L"redo"); break;
        case IDM_FIND: SetFocus(g.find); SendMessageW(g.find, EM_SETSEL, 0, -1); break;
        case IDM_COPYNODE: copyNodeText(); break;
        case IDM_QUEST_NEW: newQuest(false); break;
        case IDM_QUEST_DUP: newQuest(true); break;
        case IDM_QUEST_DEL: deleteQuest(); break;
        case IDM_QUEST_ID: changeQuestId(); break;
        case IDM_QUEST_UP: moveQuest(-1); break;
        case IDM_QUEST_DOWN: moveQuest(1); break;
        case IDM_ELEM_EDIT: editSelected(); break;
        case IDM_ELEM_ADDATTR: addAttribute(); break;
        case IDM_ELEM_RENAME: renameElement(); break;
        case IDM_ELEM_DUP: duplicateElement(); break;
        case IDM_ELEM_DEL: deleteElement(); break;
        case IDM_ELEM_UP: moveElement(-1); break;
        case IDM_ELEM_DOWN: moveElement(1); break;
        case IDM_CHECK: checkCurrent(); break;
        case IDM_COMPARE: compareDialog(); break;
        case IDM_UNPACK: unpackDialog(); break;
        case IDM_PACK: packDialog(); break;
        case IDM_NAMES: namesInfo(); break;
        case IDM_REFERENCE: showText(SchemaReferenceText()); break;
        case IDM_ABOUT:
            MessageBoxW(g.hwnd,
                L"QuestEditor\r\n\r\nEdits the quest files of DarkEden: the server's data\\SimpleGQuest.xml / EventGQuest.xml, and the client's copies "
                L"inside Data\\ui\\txt\\tutorialetc.rpk (read and written with the client's own RAR code).\r\n\r\nSee Tools\\QuestEditor\\README.md.",
                L"About QuestEditor", MB_OK | MB_ICONINFORMATION);
            break;
    }
}

LRESULT onNotify(NMHDR* nm)
{
    if (nm->hwndFrom == g.list)
    {
        if (nm->code == LVN_GETDISPINFOW)
        {
            NMLVDISPINFOW* di = (NMLVDISPINFOW*)nm;
            int row = di->item.iItem;
            if (row < 0 || row >= (int)g.shown.size() || !(di->item.mask & LVIF_TEXT)) return 0;
            const XNode* q = g.quests[(size_t)g.shown[(size_t)row]];
            static std::wstring buf;
            switch (di->item.iSubItem)
            {
                case 0: buf = q->attrValue(L"id", L"?"); break;
                case 1: { const XNode* t = q->child(L"Title"); buf = t ? XOneLine(t->text) : L""; break; }
                case 2: buf = questRace(q); break;
                case 3: buf = questLevel(q); break;
                case 4: buf = q->attrValue(L"type"); break;
                default: buf.clear();
            }
            di->item.pszText = &buf[0];
            return 0;
        }
        if (nm->code == LVN_ITEMCHANGED)
        {
            NMLISTVIEW* lv = (NMLISTVIEW*)nm;
            if (g.suppressSel) return 0;
            if ((lv->uNewState & LVIS_SELECTED) && !(lv->uOldState & LVIS_SELECTED) && lv->iItem >= 0 && lv->iItem < (int)g.shown.size())
                showQuest(g.shown[(size_t)lv->iItem]);
            return 0;
        }
        if (nm->code == LVN_KEYDOWN)
        {
            NMLVKEYDOWN* k = (NMLVKEYDOWN*)nm;
            if (k->wVKey == VK_DELETE && GetKeyState(VK_CONTROL) >= 0) deleteQuest();
            return 0;
        }
    }
    if (nm->hwndFrom == g.tree)
    {
        if (nm->code == TVN_SELCHANGEDW)
        {
            if (g.suppressSel) return 0;
            NMTREEVIEWW* tv = (NMTREEVIEWW*)nm;
            showNode((XNode*)tv->itemNew.lParam);
            return 0;
        }
        if (nm->code == TVN_KEYDOWN)
        {
            NMTVKEYDOWN* k = (NMTVKEYDOWN*)nm;
            if (k->wVKey == VK_DELETE) { deleteElement(); return 1; }
            if (k->wVKey == VK_F2) { editSelected(); return 1; }
            return 0;
        }
        if (nm->code == NM_DBLCLK)
        {
            editSelected();
            return 1;
        }
        if (nm->code == NM_RCLICK)
        {
            POINT pt;
            GetCursorPos(&pt);
            POINT cl = pt;
            ScreenToClient(g.tree, &cl);
            TVHITTESTINFO ht = {};
            ht.pt = cl;
            HTREEITEM h = TreeView_HitTest(g.tree, &ht);
            if (h) TreeView_SelectItem(g.tree, h);
            rebuildRemAttrMenu();
            TrackPopupMenu(g.elemMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, g.hwnd, nullptr);
            return 1;
        }
    }
    return 0;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            g.hwnd = hwnd;
            HINSTANCE hi = GetModuleHandleW(nullptr);
            g.lblFind = CreateWindowW(L"STATIC", L"Find", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LBL_FIND, hi, nullptr);
            g.find = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hwnd, (HMENU)IDC_FIND, hi, nullptr);
            g.list = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_OWNERDATA, 0, 0, 0, 0, hwnd, (HMENU)IDC_LIST, hi, nullptr);
            ListView_SetExtendedListViewStyle(g.list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            const wchar_t* cols[] = { L"ID", L"Title", L"Race", L"Level", L"Type" };
            int widths[] = { 52, 300, 72, 56, 70 };
            for (int i = 0; i < 5; i++)
            {
                LVCOLUMNW c = {};
                c.mask = LVCF_TEXT | LVCF_WIDTH;
                c.pszText = (LPWSTR)cols[i];
                c.cx = px(widths[i]);
                ListView_InsertColumn(g.list, i, &c);
            }
            g.tree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_FULLROWSELECT, 0, 0, 0, 0, hwnd, (HMENU)IDC_TREE, hi, nullptr);
            g.detail = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 0, 0, 0, 0, hwnd, (HMENU)IDC_DETAIL, hi, nullptr);
            SetWindowSubclass(g.detail, detailProc, 1, 0);
            g.status = CreateWindowW(STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, (HMENU)IDC_STATUS, hi, nullptr);
            for (HWND h : { g.lblFind, g.find, g.list, g.tree, g.status }) SendMessageW(h, WM_SETFONT, (WPARAM)g.uiFont, TRUE);
            SendMessageW(g.detail, WM_SETFONT, (WPARAM)g.monoFont, TRUE);
            SendMessageW(g.detail, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);
            buildMenus();
            DragAcceptFiles(hwnd, TRUE);
            showNode(nullptr);
            statusSummary(L"");
            return 0;
        }
        case WM_SIZE: layout(); return 0;
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mm = (MINMAXINFO*)lp;
            mm->ptMinTrackSize.x = px(760);
            mm->ptMinTrackSize.y = px(440);
            return 0;
        }
        case WM_COMMAND:
            if (lp == (LPARAM)g.find && HIWORD(wp) == EN_CHANGE)
            {
                g.filter = windowText(g.find);
                int keep = g.curQuest;
                rebuildQuests();
                bool still = false;
                for (int i : g.shown) if (i == keep) still = true;
                if (!still) selectQuestIndex(g.shown.empty() ? -1 : g.shown[0]);
                else selectQuestIndex(keep);
                statusSummary(L"");
                return 0;
            }
            onCommand(LOWORD(wp));
            return 0;
        case WM_NOTIFY: return onNotify((NMHDR*)lp);
        case WM_INITMENUPOPUP:
            if ((HMENU)wp == g.elemMenu) rebuildRemAttrMenu();
            return 0;
        case WM_DROPFILES:
        {
            wchar_t file[MAX_PATH * 4];
            if (DragQueryFileW((HDROP)wp, 0, file, (UINT)_countof(file))) openSpecUI(file);
            DragFinish((HDROP)wp);
            return 0;
        }
        case WM_CLOSE:
            if (!confirmDiscard()) return 0;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void makeFonts()
{
    HDC dc = GetDC(nullptr);
    g.scale100 = GetDeviceCaps(dc, LOGPIXELSX) * 100 / 96;
    ReleaseDC(nullptr, dc);
    NONCLIENTMETRICSW ncm = { sizeof(ncm) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    g.uiFont = CreateFontIndirectW(&ncm.lfMessageFont);
    LOGFONTW lf = ncm.lfMessageFont;
    wcscpy_s(lf.lfFaceName, L"Consolas");
    lf.lfHeight = -px(14);
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    g.monoFont = CreateFontIndirectW(&lf);
}

int runUI(const std::wstring& openSpecAtStart)
{
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);
    makeFonts();
    HINSTANCE hi = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hi;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"DEQuestEditor";
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassW(&wc);
    RECT wa;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int ww = std::min(px(1240), (int)(wa.right - wa.left) - px(40));
    int wh = std::min(px(760), (int)(wa.bottom - wa.top) - px(40));
    HWND hwnd = CreateWindowExW(0, L"DEQuestEditor", L"QuestEditor", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, ww, wh, nullptr, nullptr, hi, nullptr);
    ShowWindow(hwnd, SW_SHOW);
    loadNames();
    if (!openSpecAtStart.empty()) openSpecUI(openSpecAtStart);
    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0)
    {
        if (g_prompt == nullptr && TranslateAcceleratorW(g.hwnd, g.accel, &m)) continue;
        if (g_prompt == nullptr && m.message == WM_KEYDOWN && m.wParam == VK_TAB && GetKeyState(VK_CONTROL) >= 0 && IsDialogMessageW(g.hwnd, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}

//----------------------------------------------------------------------
// command line forms
//----------------------------------------------------------------------
int cliUsage()
{
    conWrite(L"QuestEditor [file.xml | pack.rpk | pack.rpk:Entry.xml]\r\n"
             L"QuestEditor --list pack.rpk\r\n"
             L"QuestEditor --unpack pack.rpk outdir\r\n"
             L"QuestEditor --pack out.rpk <folder | file...>\r\n"
             L"QuestEditor --check <file.xml | pack.rpk[:Entry.xml]>        (exit 1 when there are errors)\r\n"
             L"QuestEditor --compare <fileA> <fileB>                        (structure only; exit 1 when they differ)\r\n"
             L"QuestEditor --dump <file.xml | pack.rpk[:Entry.xml]> [out.xml] [--client]\r\n"
             L"QuestEditor --roundtrip pack.rpk                              (self-test of the pack writer)\r\n"
             L"Add --out report.txt to any command to write its output to a file instead of the console.\r\n");
    return 2;
}

int cliList(const std::wstring& rpk)
{
    std::vector<RarEntry> entries;
    std::wstring err;
    if (!RarReadAll(rpk, RPK_PASSWORD, entries, err)) { conWrite(err + L"\r\n"); return 1; }
    for (const auto& e : entries) conWrite(fmt(L"%-40s %8u bytes  method %02X  %s\r\n", str2w(e.name).c_str(), (unsigned)e.data.size(), e.method, RarDosTimeText(e.fileTime).c_str()));
    conWrite(fmt(L"%u entries\r\n", (unsigned)entries.size()));
    return 0;
}

int cliRoundtrip(const std::wstring& rpk)
{
    std::vector<RarEntry> entries;
    std::wstring err;
    if (!RarReadAll(rpk, RPK_PASSWORD, entries, err)) { conWrite(L"read: " + err + L"\r\n"); return 1; }
    conWrite(fmt(L"%u entries read from %s through the client's reader\r\n", (unsigned)entries.size(), rpk.c_str()));

    // the header CRC formula, checked against the original archive's own headers
    std::vector<uint8_t> raw;
    readFile(rpk, raw);
    int headsOk = 0, headsBad = 0;
    if (raw.size() >= 20)
    {
        size_t pos = 7;
        while (pos + 7 <= raw.size())
        {
            uint16_t crc = raw[pos] | (raw[pos + 1] << 8);
            uint8_t  type = raw[pos + 2];
            uint16_t flags = raw[pos + 3] | (raw[pos + 4] << 8);
            uint16_t size = raw[pos + 5] | (raw[pos + 6] << 8);
            if (size < 7 || pos + size > raw.size()) break;
            uint16_t mine = (uint16_t)RarCrc32(&raw[pos + 2], size - 2);
            if (mine == crc) headsOk++; else headsBad++;
            uint32_t dataSize = 0;
            if (type == 0x74 || (flags & 0x8000)) dataSize = raw[pos + 7] | (raw[pos + 8] << 8) | (raw[pos + 9] << 16) | ((uint32_t)raw[pos + 10] << 24);
            pos += size + dataSize;
        }
    }
    conWrite(fmt(L"header CRC formula matches %d of the original's %d block headers\r\n", headsOk, headsOk + headsBad));

    wchar_t tmp[MAX_PATH * 2];
    GetTempPathW((DWORD)_countof(tmp), tmp);
    std::wstring out = std::wstring(tmp) + L"QuestEditor_roundtrip.rpk";
    if (!RarWriteAll(out, entries, err)) { conWrite(L"write: " + err + L"\r\n"); return 1; }
    std::vector<RarEntry> back;
    if (!RarReadAll(out, RPK_PASSWORD, back, err)) { conWrite(L"read back: " + err + L"\r\n"); return 1; }
    int bad = 0;
    if (back.size() != entries.size()) { conWrite(fmt(L"FAIL: %u entries came back, %u went in\r\n", (unsigned)back.size(), (unsigned)entries.size())); bad++; }
    for (size_t i = 0; i < back.size() && i < entries.size(); i++)
    {
        if (back[i].name != entries[i].name || back[i].data != entries[i].data)
        {
            conWrite(fmt(L"FAIL: %s differs after the round trip\r\n", str2w(entries[i].name).c_str()));
            bad++;
        }
    }
    std::vector<uint8_t> outBytes;
    readFile(out, outBytes);
    DeleteFileW(out.c_str());
    conWrite(fmt(L"written as %u bytes (stored, not solid, not encrypted; the original is %u bytes)\r\n", (unsigned)outBytes.size(), (unsigned)raw.size()));
    conWrite(bad ? L"ROUNDTRIP FAILED\r\n" : L"ROUNDTRIP OK: every entry reads back byte for byte through the client's own RAR code\r\n");
    return bad ? 1 : 0;
}

int cliUnpack(const std::wstring& rpk, const std::wstring& dir)
{
    std::wstring report;
    bool ok = unpackTo(rpk, dir, report);
    conWrite(report + L"\r\n");
    return ok ? 0 : 1;
}

int cliPack(const std::wstring& out, const std::vector<std::wstring>& inputs)
{
    std::vector<std::wstring> files;
    if (inputs.size() == 1 && isDir(inputs[0])) files = filesInDir(inputs[0]);
    else files = inputs;
    if (files.empty()) { conWrite(L"nothing to pack\r\n"); return 1; }
    std::wstring report;
    bool ok = packFrom(files, out, report);
    conWrite(report + L"\r\n");
    return ok ? 0 : 1;
}

int cliCheck(const std::wstring& spec)
{
    Doc d;
    std::wstring err;
    if (!openSpec(spec, false, d, err)) { conWrite(spec + L": " + err + L"\r\n"); return 1; }
    NameTable names;
    names.load({ exeDir(), dirOf(d.path) });
    std::vector<CheckItem> items;
    SchemaCheck(*d.xml, d.clientStyle, names.empty() ? nullptr : &names, items);
    conWrite(L"CHECK of " + d.name() + (d.clientStyle ? L" (as the client's copy)\r\n" : L" (as the server's copy)\r\n"));
    conWrite(SchemaCheckText(items));
    for (const auto& it : items) if (it.level == CK_ERROR) return 1;
    return 0;
}

int cliCompare(const std::wstring& specA, const std::wstring& specB)
{
    Doc a, b;
    std::wstring err;
    if (!openSpec(specA, false, a, err)) { conWrite(specA + L": " + err + L"\r\n"); return 2; }
    if (!openSpec(specB, false, b, err)) { conWrite(specB + L": " + err + L"\r\n"); return 2; }
    std::wstring t = SchemaCompareText(*a.xml, a.spec(), *b.xml, b.spec());
    conWrite(t);
    // exit 1 when the structures differ
    return t.rfind(L"0 quests only in A, 0 only in B, 0 with a different structure", 0) == 0 ? 0 : 1;
}

int cliDump(const std::wstring& spec, const std::wstring& out, bool clientStyle)
{
    Doc d;
    std::wstring err;
    if (!openSpec(spec, false, d, err)) { conWrite(spec + L": " + err + L"\r\n"); return 1; }
    if (out.empty()) { conWrite(d.xml->writeText(clientStyle)); return 0; }
    std::vector<uint8_t> bytes;
    d.xml->write(bytes, clientStyle);
    if (!writeBytes(out, bytes.data(), bytes.size())) { conWrite(L"cannot write " + out + L"\r\n"); return 1; }
    conWrite(fmt(L"%u quests written to %s\r\n", (unsigned)std::count_if(d.xml->root->children.begin(), d.xml->root->children.end(), [](const std::unique_ptr<XNode>& c) { return c->name == L"Quest"; }), out.c_str()));
    return 0;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> args;
    for (int i = 1; i < argc; i++) args.push_back(argv[i]);
    LocalFree(argv);

    // --out FILE anywhere sends console output to that file
    for (size_t i = 0; i + 1 < args.size(); i++)
        if (args[i] == L"--out") { con.toFile = args[i + 1]; args.erase(args.begin() + (ptrdiff_t)i, args.begin() + (ptrdiff_t)i + 2); break; }
    bool clientStyle = false;
    for (size_t i = 0; i < args.size(); i++)
        if (args[i] == L"--client") { clientStyle = true; args.erase(args.begin() + (ptrdiff_t)i); break; }

    if (!args.empty() && args[0].rfind(L"--", 0) == 0)
    {
        conInit();
        int rc;
        const std::wstring& cmd = args[0];
        if (cmd == L"--list" && args.size() == 2) rc = cliList(args[1]);
        else if (cmd == L"--roundtrip" && args.size() == 2) rc = cliRoundtrip(args[1]);
        else if (cmd == L"--unpack" && args.size() == 3) rc = cliUnpack(args[1], args[2]);
        else if (cmd == L"--pack" && args.size() >= 3) rc = cliPack(args[1], std::vector<std::wstring>(args.begin() + 2, args.end()));
        else if (cmd == L"--check" && args.size() == 2) rc = cliCheck(args[1]);
        else if (cmd == L"--compare" && args.size() == 3) rc = cliCompare(args[1], args[2]);
        else if (cmd == L"--dump" && (args.size() == 2 || args.size() == 3)) rc = cliDump(args[1], args.size() == 3 ? args[2] : L"", clientStyle);
        else rc = cliUsage();
        conDone();
        return rc;
    }
    return runUI(args.empty() ? L"" : args[0]);
}
