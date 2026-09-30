// InfViewer - viewer / editor for DarkEden "Data\Info\*.inf" tables.
//
//   InfViewer.exe [file.inf]
//   InfViewer.exe --dump <file.inf> [out.txt] [--schema <id>]   (no UI, exit 0 when fully decoded)
//
// Left pane lists the records, right pane shows every field of the selected
// record. Schema is picked from the file name and can be overridden.
//
// Editing: double-click a field (or F2) to change it, Edit > Add record clones
// the selected record at the end of the table (keyed tables get the next free
// id), Edit > Lists appends/removes string-list lines. Every edit is re-decoded
// before it is accepted; Save writes the bytes in place.
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#include <cwctype>
#include <fstream>
#include <string>
#include <vector>

#include "InfParser.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {

enum
{
    IDC_LIST = 100, IDC_DETAIL, IDC_SCHEMA, IDC_FIND, IDC_FINDBTN, IDC_STATUS, IDC_LBL_SCHEMA, IDC_LBL_FIND,
    IDM_OPEN = 200, IDM_RELOAD, IDM_SAVE, IDM_SAVEAS, IDM_EXPORT, IDM_EXIT,
    IDM_FILEINFO, IDM_FIND, IDM_FINDNEXT, IDM_COPYREC, IDM_ABOUT,
    IDM_EDITFIELD, IDM_ADDREC, IDM_DELREC,
    IDM_LIST_ADD = 400,   // + list index
    IDM_LIST_DEL = 500    // + list index
};

struct App
{
    HWND hwnd = nullptr, list = nullptr, detail = nullptr, schema = nullptr, find = nullptr, findBtn = nullptr;
    HWND status = nullptr, lblSchema = nullptr, lblFind = nullptr;
    HMENU listMenu = nullptr;
    HFONT uiFont = nullptr, monoFont = nullptr;
    int   scale100 = 100;   // DPI percent

    std::wstring         path;
    std::vector<uint8_t> data;
    InfDoc               doc;
    std::wstring         guessed;   // schema picked from the file name
    int                  cur = -1;  // selected record
    bool                 modified = false;
    std::vector<int>     fieldLine;   // detail-pane line where each field of `cur` starts
} g;

int px(int v) { return v * g.scale100 / 100; }

std::wstring lower(std::wstring s)
{
    for (auto& ch : s) ch = (wchar_t)towlower(ch);
    return s;
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
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write((const char*)p, (std::streamsize)n);
    return f.good();
}

bool writeUtf8(const std::wstring& path, const std::wstring& text)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    std::string bytes("\xEF\xBB\xBF");
    size_t base = bytes.size();
    bytes.resize(base + (size_t)len);
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), &bytes[base], len, nullptr, nullptr);
    return writeBytes(path, bytes.data(), bytes.size());
}

std::wstring fileName(const std::wstring& path)
{
    size_t s = path.find_last_of(L"\\/");
    return s == std::wstring::npos ? path : path.substr(s + 1);
}

std::wstring oneLine(std::wstring s)
{
    for (auto& ch : s)
        if (ch == L'\r' || ch == L'\n' || ch == L'\t') ch = L' ';
    return s;
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

int countLines(const std::wstring& s)
{
    int n = 1;
    for (wchar_t ch : s) if (ch == L'\n') n++;
    return n;
}

//----------------------------------------------------------------------
// value prompt (modal window built at runtime, no resource script)
//----------------------------------------------------------------------
struct Prompt
{
    HWND wnd = nullptr, label = nullptr, edit = nullptr, ok = nullptr, cancel = nullptr;
    bool done = false, accepted = false, multiline = false;
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
                MoveWindow(p->edit, pad, pad + px(24), rc.right - pad * 2, rc.bottom - pad * 3 - px(24) - btnH, TRUE);
                MoveWindow(p->ok, rc.right - pad - btnW * 2 - px(8), rc.bottom - pad - btnH, btnW, btnH, TRUE);
                MoveWindow(p->cancel, rc.right - pad - btnW, rc.bottom - pad - btnH, btnW, btnH, TRUE);
            }
            return 0;
        case WM_COMMAND:
            if (p && LOWORD(wp) == IDOK) { p->accepted = true; p->done = true; return 0; }
            if (p && LOWORD(wp) == IDCANCEL) { p->done = true; return 0; }
            break;
        case WM_CLOSE:
            if (p) p->done = true;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool promptValue(const std::wstring& title, const std::wstring& label, std::wstring& value, bool multiline)
{
    static bool registered = false;
    HINSTANCE hi = GetModuleHandleW(nullptr);
    if (!registered)
    {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = promptProc;
        wc.hInstance     = hi;
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"DEInfPrompt";
        RegisterClassW(&wc);
        registered = true;
    }
    Prompt p;
    p.multiline = multiline;
    g_prompt = &p;

    int w = px(620), h = multiline ? px(380) : px(170);
    RECT orc;
    GetWindowRect(g.hwnd, &orc);
    int x = orc.left + ((orc.right - orc.left) - w) / 2;
    int y = orc.top + ((orc.bottom - orc.top) - h) / 2;
    p.wnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"DEInfPrompt", title.c_str(),
                            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, x, y, w, h, g.hwnd, nullptr, hi, nullptr);
    p.label = CreateWindowW(L"STATIC", label.c_str(), WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS, 0, 0, 0, 0, p.wnd, nullptr, hi, nullptr);
    DWORD es = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
    if (multiline) es = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL;
    p.edit   = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", crlf(value).c_str(), es, 0, 0, 0, 0, p.wnd, nullptr, hi, nullptr);
    p.ok     = CreateWindowW(L"BUTTON", multiline ? L"OK (Ctrl+Enter)" : L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, p.wnd, (HMENU)IDOK, hi, nullptr);
    p.cancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, p.wnd, (HMENU)IDCANCEL, hi, nullptr);
    SendMessageW(p.label, WM_SETFONT, (WPARAM)g.uiFont, TRUE);
    SendMessageW(p.edit, WM_SETFONT, (WPARAM)(multiline ? g.monoFont : g.uiFont), TRUE);
    SendMessageW(p.ok, WM_SETFONT, (WPARAM)g.uiFont, TRUE);
    SendMessageW(p.cancel, WM_SETFONT, (WPARAM)g.uiFont, TRUE);
    SendMessageW(p.edit, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);

    EnableWindow(g.hwnd, FALSE);
    ShowWindow(p.wnd, SW_SHOW);
    SendMessageW(p.wnd, WM_SIZE, 0, 0);
    SetFocus(p.edit);
    if (!multiline) SendMessageW(p.edit, EM_SETSEL, 0, -1);

    MSG m;
    while (!p.done && GetMessageW(&m, nullptr, 0, 0) > 0)
    {
        if (m.message == WM_KEYDOWN && m.wParam == VK_ESCAPE) { p.done = true; break; }
        if (m.message == WM_KEYDOWN && m.wParam == VK_RETURN && (!multiline || GetKeyState(VK_CONTROL) < 0))
        {
            p.accepted = true;
            p.done = true;
            break;
        }
        if (IsDialogMessageW(p.wnd, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    if (p.accepted)
    {
        int len = GetWindowTextLengthW(p.edit);
        std::wstring t((size_t)len + 1, L'\0');
        GetWindowTextW(p.edit, &t[0], len + 1);
        t.resize((size_t)len);
        // the file stores bare \n in multi-line text; the edit control gave us \r\n
        std::wstring o;
        for (wchar_t ch : t) if (ch != L'\r') o += ch;
        value = o;
    }
    EnableWindow(g.hwnd, TRUE);
    DestroyWindow(p.wnd);
    g_prompt = nullptr;
    SetForegroundWindow(g.hwnd);
    return p.accepted;
}

//----------------------------------------------------------------------
// UI state
//----------------------------------------------------------------------
void setStatus(const std::wstring& s)
{
    SendMessageW(g.status, SB_SETTEXTW, 0, (LPARAM)s.c_str());
}

void updateTitle()
{
    std::wstring t = L"InfViewer";
    if (!g.path.empty()) t = (g.modified ? L"*" : L"") + fileName(g.path) + L" - " + t;
    SetWindowTextW(g.hwnd, t.c_str());
}

std::wstring fileInfoText()
{
    std::wstring t;
    t += L"File    : " + g.path + L"\r\n";
    t += L"Schema  : " + g.doc.schemaTitle + L"  [" + g.doc.schemaId + L"]\r\n";
    t += L"Note    : " + g.doc.note + L"\r\n";
    wchar_t buf[256];
    swprintf_s(buf, L"Size    : %u bytes, decoded %u bytes\r\n", (unsigned)g.doc.fileSize, (unsigned)g.doc.consumed);
    t += buf;
    t += g.doc.complete() ? L"Status  : OK - every byte of the file is accounted for.\r\n"
                          : L"Status  : INCOMPLETE - see warnings. Try another schema from the drop-down.\r\n";
    for (const auto& w : g.doc.warnings) t += L"Warning : " + w + L"\r\n";
    swprintf_s(buf, L"Records : %u\r\n", (unsigned)g.doc.records.size());
    t += buf;
    t += g.doc.tableKind ? L"Editing : records can be added (Ctrl+N clones the selected one) and deleted; fields edited with F2.\r\n"
                         : L"Editing : fields can be edited with F2; this file is not a single table, so records cannot be added here.\r\n";
    if (!g.doc.header.empty())
    {
        t += L"\r\n[Header]\r\n";
        for (const auto& f : g.doc.header) t += L"  " + f.name + L" = " + f.value + L"\r\n";
    }
    t += L"\r\nSelect a record on the left to see its fields. Ctrl+F searches every field.";
    return t;
}

std::wstring recordText(const InfRecord& r, std::vector<int>& lines)
{
    std::wstring t;
    lines.clear();
    t += L"Record #" + r.key;
    wchar_t buf[64];
    swprintf_s(buf, L"   (offset 0x%X)\r\n", (unsigned)r.offset);
    t += buf;
    int line = 1;
    if (!r.summary.empty())
    {
        std::wstring s = crlf(r.summary);
        t += s + L"\r\n";
        line += countLines(s);
    }
    t += L"\r\n";
    line++;
    size_t w = 0;
    for (const auto& f : r.fields) w = max(w, f.name.size());
    for (const auto& f : r.fields)
    {
        std::wstring name = f.name;
        name.resize(w, L' ');
        std::wstring v = crlf(f.value);
        const wchar_t* tag = f.kind == INF_NONE ? L"   " : f.kind == INF_STR ? L" s " : L" # ";
        t += tag + name + L" : " + v + L"\r\n";
        lines.push_back(line);
        line += countLines(v);
    }
    return t;
}

void showDetail(const std::wstring& text)
{
    SetWindowTextW(g.detail, text.c_str());
}

void rebuildListMenu()
{
    while (GetMenuItemCount(g.listMenu) > 0) DeleteMenu(g.listMenu, 0, MF_BYPOSITION);
    if (g.cur < 0 || g.cur >= (int)g.doc.records.size() || g.doc.records[(size_t)g.cur].lists.empty())
    {
        AppendMenuW(g.listMenu, MF_STRING | MF_GRAYED, 0, L"(selected record has no string lists)");
        return;
    }
    const auto& lists = g.doc.records[(size_t)g.cur].lists;
    for (size_t i = 0; i < lists.size() && i < 100; i++)
    {
        AppendMenuW(g.listMenu, MF_STRING, IDM_LIST_ADD + (UINT_PTR)i, (L"Append line to " + lists[i].name + L"...").c_str());
        AppendMenuW(g.listMenu, MF_STRING | (lists[i].count ? 0 : MF_GRAYED), IDM_LIST_DEL + (UINT_PTR)i,
                    (L"Remove last line of " + lists[i].name).c_str());
    }
}

void showRecord(int i)
{
    g.cur = i;
    g.fieldLine.clear();
    if (i < 0 || i >= (int)g.doc.records.size())
        showDetail(fileInfoText());
    else
        showDetail(recordText(g.doc.records[(size_t)i], g.fieldLine));
    rebuildListMenu();
}

void refreshList()
{
    ListView_SetItemCountEx(g.list, (int)g.doc.records.size(), 0);
    ListView_SetItemState(g.list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    InvalidateRect(g.list, nullptr, TRUE);
}

void selectRecord(int i)
{
    if (i < 0 || i >= (int)g.doc.records.size()) { showRecord(-1); return; }
    ListView_SetItemState(g.list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(g.list, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(g.list, i, FALSE);
    showRecord(i);
}

std::wstring currentSchemaId()
{
    int sel = (int)SendMessageW(g.schema, CB_GETCURSEL, 0, 0);
    if (sel <= 0) return g.guessed;
    const auto& list = InfSchemaList();
    if ((size_t)(sel - 1) < list.size()) return list[(size_t)(sel - 1)].id;
    return g.guessed;
}

void statusSummary(const wchar_t* extra)
{
    wchar_t buf[512];
    swprintf_s(buf, L"%s%s  |  %s  |  %u records  |  %u / %u bytes decoded  |  %s%s%s",
               g.modified ? L"*" : L"", fileName(g.path).c_str(), g.doc.schemaTitle.c_str(),
               (unsigned)g.doc.records.size(), (unsigned)g.doc.consumed, (unsigned)g.doc.fileSize,
               g.doc.complete() ? L"OK" : L"INCOMPLETE - see file info", extra ? L"  |  " : L"", extra ? extra : L"");
    setStatus(buf);
}

void reparse()
{
    if (g.path.empty()) return;
    InfParse(g.data, currentSchemaId(), g.doc);
    refreshList();
    showRecord(-1);
    statusSummary(nullptr);
    updateTitle();
}

void fillSchemaCombo()
{
    SendMessageW(g.schema, CB_RESETCONTENT, 0, 0);
    std::wstring autoLabel = L"Auto";
    if (!g.guessed.empty()) autoLabel += L"  (" + g.guessed + L")";
    SendMessageW(g.schema, CB_ADDSTRING, 0, (LPARAM)autoLabel.c_str());
    for (const auto& s : InfSchemaList())
    {
        std::wstring label = std::wstring(s.id) + L"  -  " + s.title;
        SendMessageW(g.schema, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageW(g.schema, CB_SETCURSEL, 0, 0);
}

// true = go ahead (nothing unsaved, or the user chose to discard / saved)
bool saveFile(const std::wstring& to);
bool confirmDiscard()
{
    if (!g.modified) return true;
    int r = MessageBoxW(g.hwnd, (L"Save changes to " + fileName(g.path) + L"?").c_str(), L"InfViewer",
                        MB_YESNOCANCEL | MB_ICONQUESTION);
    if (r == IDCANCEL) return false;
    if (r == IDYES) return saveFile(g.path);
    return true;
}

void loadFile(const std::wstring& path)
{
    if (!confirmDiscard()) return;
    std::vector<uint8_t> data;
    if (!readFile(path, data))
    {
        MessageBoxW(g.hwnd, (L"Cannot read\r\n" + path).c_str(), L"InfViewer", MB_OK | MB_ICONERROR);
        return;
    }
    g.path     = path;
    g.data     = std::move(data);
    g.modified = false;
    g.guessed  = InfGuessSchema(path, g.data);
    fillSchemaCombo();
    reparse();
}

bool saveFile(const std::wstring& to)
{
    if (g.path.empty()) return false;
    if (!writeBytes(to, g.data.data(), g.data.size()))
    {
        MessageBoxW(g.hwnd, (L"Could not write\r\n" + to).c_str(), L"InfViewer", MB_OK | MB_ICONERROR);
        return false;
    }
    g.path     = to;
    g.modified = false;
    updateTitle();
    statusSummary((L"saved " + fileName(to)).c_str());
    return true;
}

void saveAsDialog()
{
    if (g.path.empty()) return;
    wchar_t file[MAX_PATH];
    wcscpy_s(file, fileName(g.path).c_str());
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"DarkEden info files (*.inf)\0*.inf\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrDefExt = L"inf";
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&ofn)) saveFile(file);
}

void openDialog()
{
    wchar_t file[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"DarkEden info files (*.inf)\0*.inf\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn)) loadFile(file);
}

void exportDialog()
{
    if (g.path.empty()) return;
    std::wstring stem = fileName(g.path);
    size_t dot = stem.find_last_of(L'.');
    if (dot != std::wstring::npos) stem = stem.substr(0, dot);
    wchar_t file[MAX_PATH];
    wcscpy_s(file, (stem + L".txt").c_str());
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner   = g.hwnd;
    ofn.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&ofn)) return;
    if (writeUtf8(file, InfDocToText(g.doc, g.path)))
        setStatus(std::wstring(L"Exported ") + file);
    else
        MessageBoxW(g.hwnd, L"Could not write the file.", L"InfViewer", MB_OK | MB_ICONERROR);
}

void copyRecord()
{
    int len = GetWindowTextLengthW(g.detail);
    std::wstring t((size_t)len + 1, L'\0');
    GetWindowTextW(g.detail, &t[0], len + 1);
    t.resize((size_t)len);
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
    setStatus(L"Record text copied to the clipboard.");
}

//----------------------------------------------------------------------
// editing
//----------------------------------------------------------------------
// Re-decode `next`; keep it only if the file is still fully accounted for.
bool applyImage(std::vector<uint8_t>& next, int selectAfter, const std::wstring& what)
{
    InfDoc trial;
    InfParse(next, currentSchemaId(), trial);
    if (!trial.complete())
    {
        std::wstring msg = L"The change was rejected because the file would no longer decode cleanly:\r\n\r\n";
        for (const auto& w : trial.warnings) msg += w + L"\r\n";
        MessageBoxW(g.hwnd, msg.c_str(), L"InfViewer", MB_OK | MB_ICONERROR);
        return false;
    }
    g.data     = std::move(next);
    g.doc      = std::move(trial);
    g.modified = true;
    ListView_SetItemCountEx(g.list, (int)g.doc.records.size(), LVSICF_NOSCROLL);
    InvalidateRect(g.list, nullptr, TRUE);
    selectRecord(selectAfter);
    updateTitle();
    statusSummary((what + L" - unsaved, Ctrl+S to write the file").c_str());
    return true;
}

void editField(int fieldIdx)
{
    if (g.cur < 0 || g.cur >= (int)g.doc.records.size()) return;
    const InfRecord& r = g.doc.records[(size_t)g.cur];
    if (fieldIdx < 0 || fieldIdx >= (int)r.fields.size()) return;
    const InfField& f = r.fields[(size_t)fieldIdx];
    if (f.kind == INF_NONE)
    {
        setStatus(L"\"" + f.name + L"\" is a derived value or a count and cannot be edited directly; edit its parts, or use Edit > Lists.");
        MessageBeep(MB_ICONWARNING);
        return;
    }
    std::wstring value, label;
    if (f.kind == INF_INT)
    {
        bool neg = !f.value.empty() && f.value[0] == L'-';
        value = std::to_wstring(InfReadInt(g.data, f, neg));
        wchar_t buf[128];
        swprintf_s(buf, L"  (%d-byte integer, 0x.. accepted)", f.size);
        label = f.name + buf;
    }
    else
    {
        value = InfReadStr(g.data, f);
        label = f.name + L"  (text, stored as CP949)";
    }
    if (!promptValue(L"Edit " + f.name + L"  -  record #" + r.key, label, value, f.kind == INF_STR)) return;

    std::vector<uint8_t> next;
    std::wstring err;
    if (!InfSetField(g.doc, g.data, (size_t)g.cur, (size_t)fieldIdx, value, next, err))
    {
        MessageBoxW(g.hwnd, err.c_str(), L"InfViewer", MB_OK | MB_ICONERROR);
        return;
    }
    int keep = g.cur;
    if (applyImage(next, keep, L"Changed " + f.name))
    {
        // put the caret back on the edited field
        if (fieldIdx < (int)g.fieldLine.size())
        {
            int ch = (int)SendMessageW(g.detail, EM_LINEINDEX, (WPARAM)g.fieldLine[(size_t)fieldIdx], 0);
            SendMessageW(g.detail, EM_SETSEL, (WPARAM)ch, (LPARAM)ch);
            SendMessageW(g.detail, EM_SCROLLCARET, 0, 0);
        }
    }
}

int fieldAtCaret()
{
    if (g.fieldLine.empty()) return -1;
    DWORD sel = 0;
    SendMessageW(g.detail, EM_GETSEL, (WPARAM)&sel, 0);
    int line = (int)SendMessageW(g.detail, EM_LINEFROMCHAR, (WPARAM)sel, 0);
    int idx = -1;
    for (size_t i = 0; i < g.fieldLine.size(); i++)
        if (g.fieldLine[i] <= line) idx = (int)i;
    return idx;
}

void editFieldAtCaret()
{
    if (g.cur < 0) { setStatus(L"Select a record first."); return; }
    int idx = fieldAtCaret();
    if (idx < 0) { setStatus(L"Click on a field line, then press F2 (or double-click it)."); return; }
    editField(idx);
}

void addRecord()
{
    if (g.doc.records.empty()) return;
    if (!g.doc.tableKind)
    {
        MessageBoxW(g.hwnd, L"This file is not a single record table, so the viewer cannot add records to it.", L"InfViewer", MB_OK | MB_ICONINFORMATION);
        return;
    }
    int src = g.cur >= 0 ? g.cur : (int)g.doc.records.size() - 1;
    std::vector<uint8_t> next;
    std::wstring err;
    long long newId = -1;
    if (!InfCloneRecord(g.doc, g.data, (size_t)src, next, err, &newId))
    {
        MessageBoxW(g.hwnd, err.c_str(), L"InfViewer", MB_OK | MB_ICONERROR);
        return;
    }
    std::wstring what = L"Added record (copy of #" + g.doc.records[(size_t)src].key + L")";
    if (newId >= 0) what += L" with new id " + std::to_wstring(newId);
    else what += L" at the end of the table";
    if (applyImage(next, (int)g.doc.records.size(), what))
        setStatus(what + L" - now edit its fields (F2). Unsaved, Ctrl+S to write the file.");
}

void deleteRecord()
{
    if (g.cur < 0 || g.cur >= (int)g.doc.records.size()) return;
    if (!g.doc.tableKind)
    {
        MessageBoxW(g.hwnd, L"This file is not a single record table, so the viewer cannot delete records from it.", L"InfViewer", MB_OK | MB_ICONINFORMATION);
        return;
    }
    const InfRecord& r = g.doc.records[(size_t)g.cur];
    std::wstring msg = L"Delete record #" + r.key + L" (" + oneLine(r.summary) + L")?";
    if (g.doc.tableKind == 1) msg += L"\r\n\r\nThis table is indexed by position: every record after it moves up by one, which changes the ids the game uses.";
    if (MessageBoxW(g.hwnd, msg.c_str(), L"InfViewer", MB_OKCANCEL | MB_ICONWARNING) != IDOK) return;
    std::vector<uint8_t> next;
    std::wstring err;
    if (!InfDeleteRecord(g.doc, g.data, (size_t)g.cur, next, err))
    {
        MessageBoxW(g.hwnd, err.c_str(), L"InfViewer", MB_OK | MB_ICONERROR);
        return;
    }
    int keep = g.cur;
    applyImage(next, min(keep, (int)g.doc.records.size() - 2), L"Deleted record");
}

void listAppend(size_t li)
{
    if (g.cur < 0 || g.cur >= (int)g.doc.records.size()) return;
    const InfRecord& r = g.doc.records[(size_t)g.cur];
    if (li >= r.lists.size()) return;
    std::wstring value;
    if (!promptValue(L"Append to " + r.lists[li].name + L"  -  record #" + r.key, L"New line (text, stored as CP949)", value, true)) return;
    std::vector<uint8_t> next;
    std::wstring err;
    if (!InfListAppend(g.doc, g.data, (size_t)g.cur, li, value, next, err))
    {
        MessageBoxW(g.hwnd, err.c_str(), L"InfViewer", MB_OK | MB_ICONERROR);
        return;
    }
    applyImage(next, g.cur, L"Appended a line to " + r.lists[li].name);
}

void listRemoveLast(size_t li)
{
    if (g.cur < 0 || g.cur >= (int)g.doc.records.size()) return;
    const InfRecord& r = g.doc.records[(size_t)g.cur];
    if (li >= r.lists.size()) return;
    std::vector<uint8_t> next;
    std::wstring err;
    if (!InfListRemoveLast(g.doc, g.data, (size_t)g.cur, li, next, err))
    {
        MessageBoxW(g.hwnd, err.c_str(), L"InfViewer", MB_OK | MB_ICONERROR);
        return;
    }
    applyImage(next, g.cur, L"Removed the last line of " + r.lists[li].name);
}

//----------------------------------------------------------------------
// find
//----------------------------------------------------------------------
void highlightInDetail(const std::wstring& needle)
{
    int len = GetWindowTextLengthW(g.detail);
    std::wstring t((size_t)len + 1, L'\0');
    GetWindowTextW(g.detail, &t[0], len + 1);
    t.resize((size_t)len);
    size_t at = lower(t).find(needle);
    if (at == std::wstring::npos) return;
    SendMessageW(g.detail, EM_SETSEL, (WPARAM)at, (LPARAM)(at + needle.size()));
    SendMessageW(g.detail, EM_SCROLLCARET, 0, 0);
}

bool recordMatches(const InfRecord& r, const std::wstring& needle, std::wstring& where)
{
    if (lower(r.key).find(needle) != std::wstring::npos) { where = L"key"; return true; }
    if (lower(r.summary).find(needle) != std::wstring::npos) { where = L"summary"; return true; }
    for (const auto& f : r.fields)
    {
        if (lower(f.value).find(needle) != std::wstring::npos || lower(f.name).find(needle) != std::wstring::npos)
        {
            where = f.name;
            return true;
        }
    }
    return false;
}

void doFind(bool next)
{
    wchar_t buf[512];
    GetWindowTextW(g.find, buf, _countof(buf));
    std::wstring needle = lower(buf);
    if (needle.empty() || g.doc.records.empty())
    {
        SetFocus(g.find);
        return;
    }
    int n     = (int)g.doc.records.size();
    int start = next ? g.cur + 1 : 0;
    for (int k = 0; k < n; k++)
    {
        int i = (start + k) % n;
        std::wstring where;
        if (recordMatches(g.doc.records[(size_t)i], needle, where))
        {
            selectRecord(i);
            highlightInDetail(needle);
            swprintf_s(buf, L"Found \"%s\" in record #%s (%s). F3 = next match.", needle.c_str(),
                       g.doc.records[(size_t)i].key.c_str(), where.c_str());
            setStatus(buf);
            return;
        }
    }
    swprintf_s(buf, L"\"%s\" not found in any record.", needle.c_str());
    setStatus(buf);
}

//----------------------------------------------------------------------
// window plumbing
//----------------------------------------------------------------------
LRESULT CALLBACK findEditProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR)
{
    if (m == WM_KEYDOWN && w == VK_RETURN)
    {
        doFind(GetKeyState(VK_SHIFT) < 0 ? false : true);
        return 0;
    }
    if (m == WM_CHAR && w == VK_RETURN) return 0;   // no beep
    return DefSubclassProc(h, m, w, l);
}

LRESULT CALLBACK detailProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR)
{
    if (m == WM_LBUTTONDBLCLK)
    {
        // place the caret where the user clicked, then edit that field
        LRESULT ch = SendMessageW(h, EM_CHARFROMPOS, 0, l);
        int idx = (int)LOWORD(ch);
        SendMessageW(h, EM_SETSEL, (WPARAM)idx, (LPARAM)idx);
        editFieldAtCaret();
        return 0;
    }
    if (m == WM_KEYDOWN && (w == VK_F2 || w == VK_RETURN))
    {
        editFieldAtCaret();
        return 0;
    }
    if (m == WM_CHAR && w == VK_RETURN) return 0;
    return DefSubclassProc(h, m, w, l);
}

LRESULT CALLBACK listProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR)
{
    if (m == WM_KEYDOWN && w == VK_DELETE) { deleteRecord(); return 0; }
    if (m == WM_KEYDOWN && w == VK_RETURN)
    {
        SetFocus(g.detail);
        if (!g.fieldLine.empty())
        {
            int ch = (int)SendMessageW(g.detail, EM_LINEINDEX, (WPARAM)g.fieldLine[0], 0);
            SendMessageW(g.detail, EM_SETSEL, (WPARAM)ch, (LPARAM)ch);
        }
        return 0;
    }
    return DefSubclassProc(h, m, w, l);
}

void layout()
{
    RECT rc;
    GetClientRect(g.hwnd, &rc);
    SendMessageW(g.status, WM_SIZE, 0, 0);
    RECT sb;
    GetWindowRect(g.status, &sb);
    int statusH = sb.bottom - sb.top;

    int pad = px(6), rowH = px(24), top = px(8);
    int x = pad;
    MoveWindow(g.lblSchema, x, top + px(4), px(52), rowH, TRUE);
    x += px(52);
    int comboW = max(px(300), (rc.right - rc.left) * 45 / 100);
    MoveWindow(g.schema, x, top, comboW, px(400), TRUE);
    x += comboW + pad * 2;
    MoveWindow(g.lblFind, x, top + px(4), px(36), rowH, TRUE);
    x += px(36);
    int btnW = px(70);
    int findW = max(px(120), rc.right - x - btnW - pad * 2);
    MoveWindow(g.find, x, top, findW, rowH, TRUE);
    x += findW + pad;
    MoveWindow(g.findBtn, x, top, btnW, rowH, TRUE);

    int y = top + rowH + pad;
    int h = rc.bottom - y - statusH - pad;
    int listW = (rc.right - rc.left) * 40 / 100;
    MoveWindow(g.list, pad, y, listW - pad, h, TRUE);
    MoveWindow(g.detail, listW + pad, y, rc.right - listW - pad * 2, h, TRUE);

    int colW = listW - pad - px(4);
    ListView_SetColumnWidth(g.list, 0, px(70));
    ListView_SetColumnWidth(g.list, 2, px(70));
    ListView_SetColumnWidth(g.list, 1, max(px(80), colW - px(140) - GetSystemMetrics(SM_CXVSCROLL)));
}

void createControls()
{
    NONCLIENTMETRICSW ncm = { sizeof(ncm) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    g.uiFont = CreateFontIndirectW(&ncm.lfMessageFont);
    LOGFONTW lf = {};
    lf.lfHeight = -px(14);
    wcscpy_s(lf.lfFaceName, L"Consolas");
    lf.lfCharSet = DEFAULT_CHARSET;
    g.monoFont = CreateFontIndirectW(&lf);

    HINSTANCE hi = GetModuleHandleW(nullptr);
    g.lblSchema = CreateWindowW(L"STATIC", L"Schema:", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, g.hwnd, (HMENU)IDC_LBL_SCHEMA, hi, nullptr);
    g.schema    = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST, 0, 0, 0, 0, g.hwnd, (HMENU)IDC_SCHEMA, hi, nullptr);
    g.lblFind   = CreateWindowW(L"STATIC", L"Find:", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, g.hwnd, (HMENU)IDC_LBL_FIND, hi, nullptr);
    g.find      = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, g.hwnd, (HMENU)IDC_FIND, hi, nullptr);
    g.findBtn   = CreateWindowW(L"BUTTON", L"Find next", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0, g.hwnd, (HMENU)IDC_FINDBTN, hi, nullptr);
    g.list      = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                  0, 0, 0, 0, g.hwnd, (HMENU)IDC_LIST, hi, nullptr);
    g.detail    = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                                  0, 0, 0, 0, g.hwnd, (HMENU)IDC_DETAIL, hi, nullptr);
    g.status    = CreateWindowW(STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, g.hwnd, (HMENU)IDC_STATUS, hi, nullptr);

    for (HWND h : { g.lblSchema, g.schema, g.lblFind, g.find, g.findBtn, g.list, g.status })
        SendMessageW(h, WM_SETFONT, (WPARAM)g.uiFont, TRUE);
    SendMessageW(g.detail, WM_SETFONT, (WPARAM)g.monoFont, TRUE);
    SendMessageW(g.detail, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);

    ListView_SetExtendedListViewStyle(g.list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.pszText = (LPWSTR)L"#";        col.cx = px(70);  ListView_InsertColumn(g.list, 0, &col);
    col.pszText = (LPWSTR)L"Summary";  col.cx = px(200); ListView_InsertColumn(g.list, 1, &col);
    col.pszText = (LPWSTR)L"Offset";   col.cx = px(70);  ListView_InsertColumn(g.list, 2, &col);

    SetWindowSubclass(g.find, findEditProc, 1, 0);
    SetWindowSubclass(g.detail, detailProc, 2, 0);
    SetWindowSubclass(g.list, listProc, 3, 0);
    fillSchemaCombo();
    showDetail(L"File > Open (Ctrl+O), or drop an .inf file onto this window.\r\n\r\n"
               L"The schema is chosen from the file name. If a file decodes INCOMPLETE,\r\n"
               L"pick another schema from the drop-down, or fall back to the string scan / hex dump.\r\n\r\n"
               L"Editing: double-click a field (or F2) to change it. Fields marked # are numbers,\r\n"
               L"s are strings; unmarked ones are derived and read-only. Edit > Add record clones the\r\n"
               L"selected record onto the end of the table, Edit > Lists adds script lines.\r\n"
               L"Ctrl+S saves.");
    setStatus(L"Ready");
}

HMENU buildMenu()
{
    HMENU bar = CreateMenu();
    HMENU file = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, IDM_OPEN, L"&Open...\tCtrl+O");
    AppendMenuW(file, MF_STRING, IDM_RELOAD, L"&Reload from disk\tF5");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, IDM_SAVE, L"&Save\tCtrl+S");
    AppendMenuW(file, MF_STRING, IDM_SAVEAS, L"Save &As...\tCtrl+Shift+S");
    AppendMenuW(file, MF_STRING, IDM_EXPORT, L"&Export decoded text...\tCtrl+E");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, IDM_EXIT, L"E&xit");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)file, L"&File");

    HMENU edit = CreatePopupMenu();
    AppendMenuW(edit, MF_STRING, IDM_EDITFIELD, L"Edit &value at cursor...\tF2");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, IDM_ADDREC, L"&Add record (clone of selected)\tCtrl+N");
    AppendMenuW(edit, MF_STRING, IDM_DELREC, L"&Delete selected record\tDel");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    g.listMenu = CreatePopupMenu();
    AppendMenuW(edit, MF_POPUP, (UINT_PTR)g.listMenu, L"&Lists");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)edit, L"&Edit");

    HMENU view = CreatePopupMenu();
    AppendMenuW(view, MF_STRING, IDM_FILEINFO, L"File &info\tCtrl+I");
    AppendMenuW(view, MF_STRING, IDM_FIND, L"&Find...\tCtrl+F");
    AppendMenuW(view, MF_STRING, IDM_FINDNEXT, L"Find &next\tF3");
    AppendMenuW(view, MF_STRING, IDM_COPYREC, L"&Copy record text\tCtrl+Shift+C");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)view, L"&View");
    HMENU help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, IDM_ABOUT, L"&About");
    AppendMenuW(bar, MF_POPUP, (UINT_PTR)help, L"&Help");
    return bar;
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_CREATE:
            g.hwnd = hwnd;
            createControls();
            DragAcceptFiles(hwnd, TRUE);
            return 0;

        case WM_SIZE:
            layout();
            return 0;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi = (MINMAXINFO*)lp;
            mmi->ptMinTrackSize.x = px(640);
            mmi->ptMinTrackSize.y = px(360);
            return 0;
        }

        case WM_DROPFILES:
        {
            wchar_t file[MAX_PATH];
            if (DragQueryFileW((HDROP)wp, 0, file, MAX_PATH)) loadFile(file);
            DragFinish((HDROP)wp);
            return 0;
        }

        case WM_NOTIFY:
        {
            NMHDR* nm = (NMHDR*)lp;
            if (nm->idFrom == IDC_LIST && nm->code == LVN_GETDISPINFOW)
            {
                NMLVDISPINFOW* di = (NMLVDISPINFOW*)lp;
                static wchar_t buf[1024];
                int i = di->item.iItem;
                if (i < 0 || i >= (int)g.doc.records.size() || !(di->item.mask & LVIF_TEXT)) return 0;
                const InfRecord& r = g.doc.records[(size_t)i];
                std::wstring s;
                switch (di->item.iSubItem)
                {
                    case 0: s = r.key; break;
                    case 1: s = oneLine(r.summary); break;
                    case 2: swprintf_s(buf, L"0x%X", (unsigned)r.offset); s = buf; break;
                }
                wcsncpy_s(buf, s.c_str(), _TRUNCATE);
                di->item.pszText = buf;
                return 0;
            }
            if (nm->idFrom == IDC_LIST && nm->code == LVN_ITEMCHANGED)
            {
                NMLISTVIEW* lv = (NMLISTVIEW*)lp;
                if ((lv->uNewState & LVIS_SELECTED) && !(lv->uOldState & LVIS_SELECTED)) showRecord(lv->iItem);
                return 0;
            }
            if (nm->idFrom == IDC_LIST && nm->code == LVN_ODFINDITEMW)
            {
                NMLVFINDITEMW* fi = (NMLVFINDITEMW*)lp;
                if (!(fi->lvfi.flags & LVFI_STRING) || !fi->lvfi.psz) return -1;
                std::wstring want = lower(fi->lvfi.psz);
                int n = (int)g.doc.records.size();
                for (int k = 0; k < n; k++)
                {
                    int i = (fi->iStart + k) % n;
                    if (lower(g.doc.records[(size_t)i].key).compare(0, want.size(), want) == 0) return i;
                }
                return -1;
            }
            break;
        }

        case WM_COMMAND:
        {
            int id = LOWORD(wp), code = HIWORD(wp);
            if (id == IDC_SCHEMA && code == CBN_SELCHANGE) { reparse(); return 0; }
            if (id == IDC_FINDBTN) { doFind(true); return 0; }
            if (id >= IDM_LIST_ADD && id < IDM_LIST_ADD + 100) { listAppend((size_t)(id - IDM_LIST_ADD)); return 0; }
            if (id >= IDM_LIST_DEL && id < IDM_LIST_DEL + 100) { listRemoveLast((size_t)(id - IDM_LIST_DEL)); return 0; }
            switch (id)
            {
                case IDM_OPEN:      openDialog(); return 0;
                case IDM_RELOAD:    if (!g.path.empty()) loadFile(g.path); return 0;
                case IDM_SAVE:      if (!g.path.empty()) saveFile(g.path); return 0;
                case IDM_SAVEAS:    saveAsDialog(); return 0;
                case IDM_EXPORT:    exportDialog(); return 0;
                case IDM_EXIT:      SendMessageW(hwnd, WM_CLOSE, 0, 0); return 0;
                case IDM_EDITFIELD: editFieldAtCaret(); return 0;
                case IDM_ADDREC:    addRecord(); return 0;
                case IDM_DELREC:    deleteRecord(); return 0;
                case IDM_FILEINFO:  ListView_SetItemState(g.list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED); showRecord(-1); return 0;
                case IDM_FIND:      SetFocus(g.find); SendMessageW(g.find, EM_SETSEL, 0, -1); return 0;
                case IDM_FINDNEXT:  doFind(true); return 0;
                case IDM_COPYREC:   copyRecord(); return 0;
                case IDM_ABOUT:
                    MessageBoxW(hwnd,
                                L"InfViewer - DarkEden .inf table reader / editor\r\n\r\n"
                                L"Decodes the binary Data\\Info tables with the same field layouts the client uses "
                                L"(UI/game/*Table.cpp LoadFromFile), so you can read which ACTIONINFO uses which "
                                L"EffectSpriteType, FrameID, SoundID and so on.\r\n\r\n"
                                L"Edits rewrite the record (length prefixes included) and are only kept when the whole "
                                L"file still decodes.",
                                L"About InfViewer", MB_OK | MB_ICONINFORMATION);
                    return 0;
            }
            break;
        }

        case WM_CLOSE:
            if (!confirmDiscard()) return 0;
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int runDump(int argc, wchar_t** argv)
{
    std::wstring in, out, schema;
    for (int i = 2; i < argc; i++)
    {
        std::wstring a = argv[i];
        if (a == L"--schema" && i + 1 < argc) schema = argv[++i];
        else if (in.empty()) in = a;
        else if (out.empty()) out = a;
    }
    if (in.empty()) return 1;
    if (out.empty()) out = in + L".txt";
    std::vector<uint8_t> data;
    if (!readFile(in, data)) return 1;
    if (schema.empty()) schema = InfGuessSchema(in, data);
    InfDoc doc;
    InfParse(data, schema, doc);
    if (!writeUtf8(out, InfDocToText(doc, in))) return 1;
    return doc.complete() ? 0 : 2;
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc >= 2 && std::wstring(argv[1]) == L"--dump") return runDump(argc, argv);

    typedef BOOL(WINAPI * SetDpiAwareFn)();
    if (HMODULE u32 = GetModuleHandleW(L"user32.dll"))
        if (auto fn = (SetDpiAwareFn)GetProcAddress(u32, "SetProcessDPIAware")) fn();
    {
        HDC dc = GetDC(nullptr);
        g.scale100 = GetDeviceCaps(dc, LOGPIXELSY) * 100 / 96;
        ReleaseDC(nullptr, dc);
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"DEInfViewer";
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(wc.lpszClassName, L"InfViewer", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                              px(1180), px(720), nullptr, buildMenu(), hInst, nullptr);
    ShowWindow(hwnd, nCmdShow);

    if (argc >= 2) loadFile(argv[1]);
    LocalFree(argv);

    ACCEL acc[] = {
        { FCONTROL | FVIRTKEY, 'O', IDM_OPEN },
        { FVIRTKEY, VK_F5, IDM_RELOAD },
        { FCONTROL | FVIRTKEY, 'S', IDM_SAVE },
        { FCONTROL | FSHIFT | FVIRTKEY, 'S', IDM_SAVEAS },
        { FCONTROL | FVIRTKEY, 'E', IDM_EXPORT },
        { FCONTROL | FVIRTKEY, 'N', IDM_ADDREC },
        { FCONTROL | FVIRTKEY, 'I', IDM_FILEINFO },
        { FCONTROL | FVIRTKEY, 'F', IDM_FIND },
        { FVIRTKEY, VK_F3, IDM_FINDNEXT },
        { FCONTROL | FSHIFT | FVIRTKEY, 'C', IDM_COPYREC },
    };
    HACCEL hacc = CreateAcceleratorTableW(acc, _countof(acc));

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0)
    {
        if (!g_prompt && TranslateAcceleratorW(hwnd, hacc, &m)) continue;
        // Tab / Shift+Tab move between the find box, the record list and the detail pane
        if (!g_prompt && m.message == WM_KEYDOWN && m.wParam == VK_TAB && IsDialogMessageW(hwnd, &m)) continue;
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return (int)m.wParam;
}
