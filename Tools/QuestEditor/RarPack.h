// RarPack.h - read and write the client's .rpk packs.
//
// An .rpk is a RAR 2.0 archive (the client opens it with the UniquE RAR
// library, password "cjdgorkr"). Reading goes through that very library, so
// what the editor sees is what the game sees. Writing produces a RAR 2.0
// archive the same library reads back: every entry is stored (method 0x30),
// the archive is not solid, and entries are not encrypted - the library only
// decrypts whole 16-byte blocks, so an encrypted stored file would have to be
// padded, and padding is not acceptable for the binary entries a pack can hold.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct RarEntry
{
    std::string          name;      // as stored in the archive (CP949 / ASCII)
    std::vector<uint8_t> data;      // unpacked bytes
    uint32_t             fileTime = 0;   // DOS date/time
    uint32_t             fileAttr = 0x20;
    uint8_t              method   = 0x30;  // as found in the source archive
    bool                 encrypted = false;
};

extern const char* const RPK_PASSWORD;   // the client's password for its .rpk files

// Lists and unpacks every entry. `err` explains a failure.
bool RarReadAll(const std::wstring& path, const std::string& password, std::vector<RarEntry>& out, std::wstring& err);
// Unpacks one entry by name (case-insensitive).
bool RarReadOne(const std::wstring& path, const std::string& password, const std::string& name, std::vector<uint8_t>& out, std::wstring& err);
// Writes a new archive holding `entries` in order (stored, not solid, not encrypted).
bool RarWriteAll(const std::wstring& path, const std::vector<RarEntry>& entries, std::wstring& err);
// Serialises the archive to memory (what RarWriteAll writes).
void RarBuild(const std::vector<RarEntry>& entries, std::vector<uint8_t>& out);

uint32_t RarCrc32(const void* p, size_t n, uint32_t start = 0);
uint32_t RarDosTimeNow();
std::wstring RarDosTimeText(uint32_t t);
