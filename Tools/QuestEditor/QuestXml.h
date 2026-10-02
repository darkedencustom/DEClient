// QuestXml.h - the quest XML document: a small element tree, a tolerant
// parser, and a writer that produces what both XML readers accept:
//
//  * the game server parses with xerces (a real XML parser: entities are
//    decoded, comments allowed, text segments trimmed and concatenated);
//  * the client parses with SXml/SXml.cpp, a hand-written scanner: no entity
//    decoding, no comments, attribute values end at the first " or ', text is
//    everything up to the next '<' with leading " </>\r\t\n=\"'" stripped.
//
// Text is held as UTF-16; the file encoding (UTF-8 or CP949 "euc-kr") is
// detected on load and reused on save.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct XAttr
{
    std::wstring name, value;
};

struct XNode
{
    std::wstring       name;
    std::vector<XAttr> attrs;
    std::wstring       text;        // trimmed, newlines kept
    std::vector<std::unique_ptr<XNode>> children;
    XNode* parent = nullptr;

    const XAttr* attr(const std::wstring& n) const;
    XAttr*       attr(const std::wstring& n);
    std::wstring attrValue(const std::wstring& n, const std::wstring& def = L"") const;
    long         attrInt(const std::wstring& n, long def = -1) const;   // -1 when missing / not a number
    void         setAttr(const std::wstring& n, const std::wstring& v);  // adds at the end when new
    bool         removeAttr(const std::wstring& n);
    const XNode* child(const std::wstring& n) const;
    XNode*       child(const std::wstring& n);
    XNode*       addChild(const std::wstring& n);                      // appended
    XNode*       insertChild(size_t at, std::unique_ptr<XNode> c);
    std::unique_ptr<XNode> removeChild(size_t at);
    size_t       indexOf(const XNode* c) const;                        // SIZE_MAX when absent
    std::unique_ptr<XNode> clone() const;
};

enum XEncoding { XENC_UTF8 = 0, XENC_CP949 = 1 };

struct XDoc
{
    std::unique_ptr<XNode> root;
    XEncoding encoding = XENC_UTF8;
    bool      bom      = false;      // the file started with a UTF-8 BOM
    bool      crlf     = true;       // line ending to write
    std::wstring declaration;        // the <?xml ...?> line as read (informational)
    std::vector<std::wstring> warnings;   // comments dropped, oddities seen while parsing

    bool parse(const std::vector<uint8_t>& bytes, std::wstring& err);
    // clientStyle: raw text (no entity escaping) for the client's scanner.
    void write(std::vector<uint8_t>& out, bool clientStyle) const;
    std::wstring writeText(bool clientStyle) const;
    std::unique_ptr<XDoc> clone() const;
};

// Encoding helpers
bool        XIsValidUtf8(const uint8_t* p, size_t n);
std::wstring XBytesToWide(const uint8_t* p, size_t n, XEncoding enc);
bool        XWideToBytes(const std::wstring& w, XEncoding enc, std::string& out, std::wstring* badChars = nullptr);
std::wstring XTrim(const std::wstring& s);
std::wstring XOneLine(const std::wstring& s, size_t maxLen = 0);
std::wstring XEscape(const std::wstring& s, bool attribute);
