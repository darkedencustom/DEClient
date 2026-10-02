// RarPack.cpp - see RarPack.h
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "RarPack.h"
#include "urarlib.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

const char* const RPK_PASSWORD = "cjdgorkr";   // VS_UI_filepath.h RPK_PASSWORD

namespace {

// urarlib takes narrow paths. Use the ANSI spelling when it is exact, else the
// 8.3 short name so that any Unicode path still opens.
std::string narrowPath(const std::wstring& w)
{
    BOOL bad = FALSE;
    int len = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, w.c_str(), -1, nullptr, 0, nullptr, &bad);
    if (len > 0 && !bad)
    {
        std::string s((size_t)len, '\0');
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, w.c_str(), -1, &s[0], len, nullptr, nullptr);
        s.resize((size_t)len - 1);
        return s;
    }
    wchar_t shortBuf[MAX_PATH * 2];
    DWORD n = GetShortPathNameW(w.c_str(), shortBuf, (DWORD)(sizeof(shortBuf) / sizeof(shortBuf[0])));
    if (n == 0 || n >= sizeof(shortBuf) / sizeof(shortBuf[0])) return std::string();
    len = WideCharToMultiByte(CP_ACP, 0, shortBuf, -1, nullptr, 0, nullptr, nullptr);
    std::string s((size_t)len, '\0');
    WideCharToMultiByte(CP_ACP, 0, shortBuf, -1, &s[0], len, nullptr, nullptr);
    s.resize((size_t)len - 1);
    return s;
}

void freeList(ArchiveList_struct* list)
{
    while (list)
    {
        ArchiveList_struct* next = list->next;
        free(list->item.Name);
        free(list);
        list = next;
    }
}

uint32_t crcTable[256];
void initCrc()
{
    static bool done = false;
    if (done) return;
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) c = (c & 1) ? (c >> 1) ^ 0xEDB88320u : (c >> 1);
        crcTable[i] = c;
    }
    done = true;
}

void putU16(std::vector<uint8_t>& o, uint16_t v) { o.push_back((uint8_t)v); o.push_back((uint8_t)(v >> 8)); }
void putU32(std::vector<uint8_t>& o, uint32_t v) { for (int i = 0; i < 4; i++) o.push_back((uint8_t)(v >> (8 * i))); }

} // namespace

uint32_t RarCrc32(const void* p, size_t n, uint32_t start)
{
    initCrc();
    uint32_t c = start ^ 0xFFFFFFFFu;
    const uint8_t* b = (const uint8_t*)p;
    for (size_t i = 0; i < n; i++) c = crcTable[(uint8_t)c ^ b[i]] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

uint32_t RarDosTimeNow()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    int y = st.wYear < 1980 ? 1980 : st.wYear;
    return ((uint32_t)(y - 1980) << 25) | ((uint32_t)st.wMonth << 21) | ((uint32_t)st.wDay << 16) |
           ((uint32_t)st.wHour << 11) | ((uint32_t)st.wMinute << 5) | ((uint32_t)st.wSecond / 2);
}

std::wstring RarDosTimeText(uint32_t t)
{
    wchar_t buf[64];
    swprintf_s(buf, L"%04u-%02u-%02u %02u:%02u", 1980 + (t >> 25), (t >> 21) & 15, (t >> 16) & 31, (t >> 11) & 31, (t >> 5) & 63);
    return buf;
}

bool RarReadAll(const std::wstring& path, const std::string& password, std::vector<RarEntry>& out, std::wstring& err)
{
    out.clear();
    std::string np = narrowPath(path);
    if (np.empty()) { err = L"The path cannot be passed to the RAR reader."; return false; }
    FILE* probe = fopen(np.c_str(), "rb");
    if (!probe) { err = L"Cannot open the file."; return false; }
    fclose(probe);

    ArchiveList_struct* list = nullptr;
    urarlib_list(np.c_str(), &list);
    if (!list) { err = L"Not a RAR 2.0 archive the client library can list (or the archive is empty)."; return false; }

    bool ok = true;
    for (ArchiveList_struct* p = list; p; p = p->next)
    {
        RarEntry e;
        e.name      = p->item.Name ? p->item.Name : "";
        e.fileTime  = p->item.FileTime;
        e.fileAttr  = p->item.FileAttr;
        e.method    = p->item.Method;
        // urarlib_list does not expose the header flags; a non-stored method or a
        // password-protected pack both read fine, the flag is informational only.
        e.encrypted = false;
        char* buf = nullptr;
        int   size = 0;
        if (!urarlib_get(&buf, &size, e.name.c_str(), np.c_str(), password.c_str()))
        {
            err = L"Could not unpack \"" + std::wstring(e.name.begin(), e.name.end()) +
                  L"\" (wrong password, unsupported RAR version, or a damaged pack).";
            ok = false;
            free(buf);
            break;
        }
        if (size > 0 && buf) e.data.assign((uint8_t*)buf, (uint8_t*)buf + size);
        free(buf);
        out.push_back(std::move(e));
    }
    freeList(list);
    if (!ok) out.clear();
    return ok;
}

bool RarReadOne(const std::wstring& path, const std::string& password, const std::string& name, std::vector<uint8_t>& out, std::wstring& err)
{
    out.clear();
    std::string np = narrowPath(path);
    if (np.empty()) { err = L"The path cannot be passed to the RAR reader."; return false; }
    char* buf = nullptr;
    int   size = 0;
    if (!urarlib_get(&buf, &size, name.c_str(), np.c_str(), password.c_str()))
    {
        free(buf);
        err = L"\"" + std::wstring(name.begin(), name.end()) + L"\" could not be unpacked from the pack.";
        return false;
    }
    if (size > 0 && buf) out.assign((uint8_t*)buf, (uint8_t*)buf + size);
    free(buf);
    return true;
}

void RarBuild(const std::vector<RarEntry>& entries, std::vector<uint8_t>& out)
{
    out.clear();
    // mark head
    static const uint8_t mark[7] = { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00 };
    out.insert(out.end(), mark, mark + 7);

    // main head: CRC16, type 0x73, flags 0 (not solid), size 13, reserved
    {
        std::vector<uint8_t> h;
        h.push_back(0x73);
        putU16(h, 0x0000);
        putU16(h, 13);
        putU16(h, 0);
        putU32(h, 0);
        uint16_t crc = (uint16_t)RarCrc32(h.data(), h.size());
        putU16(out, crc);
        out.insert(out.end(), h.begin(), h.end());
    }

    for (const RarEntry& e : entries)
    {
        std::vector<uint8_t> h;
        h.push_back(0x74);                              // FILE_HEAD
        putU16(h, 0x8000 | 0x0040);                     // LONG_BLOCK | 128K window (no password, not solid)
        putU16(h, (uint16_t)(32 + e.name.size()));      // HeadSize
        putU32(h, (uint32_t)e.data.size());             // PackSize
        putU32(h, (uint32_t)e.data.size());             // UnpSize
        h.push_back(2);                                 // HostOS = Win32
        putU32(h, RarCrc32(e.data.data(), e.data.size()));
        putU32(h, e.fileTime ? e.fileTime : RarDosTimeNow());
        h.push_back(20);                                // UnpVer 2.0
        h.push_back(0x30);                              // stored
        putU16(h, (uint16_t)e.name.size());
        putU32(h, e.fileAttr);
        h.insert(h.end(), e.name.begin(), e.name.end());
        uint16_t crc = (uint16_t)RarCrc32(h.data(), h.size());
        putU16(out, crc);
        out.insert(out.end(), h.begin(), h.end());
        out.insert(out.end(), e.data.begin(), e.data.end());
    }
}

bool RarWriteAll(const std::wstring& path, const std::vector<RarEntry>& entries, std::wstring& err)
{
    for (const RarEntry& e : entries)
    {
        if (e.name.empty() || e.name.size() > 255) { err = L"An entry has no name or a name over 255 bytes."; return false; }
        if (e.data.size() > 0x7FFFFFFFu) { err = L"An entry is too large for a RAR 2.0 header."; return false; }
    }
    std::vector<uint8_t> bytes;
    RarBuild(entries, bytes);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) { err = L"Cannot write " + path; return false; }
    DWORD wrote = 0;
    BOOL ok = WriteFile(h, bytes.data(), (DWORD)bytes.size(), &wrote, nullptr);
    CloseHandle(h);
    if (!ok || wrote != bytes.size()) { err = L"Writing " + path + L" failed part way."; return false; }
    return true;
}
