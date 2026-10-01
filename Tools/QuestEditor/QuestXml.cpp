// QuestXml.cpp - see QuestXml.h
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "QuestXml.h"

#include <cwctype>

//----------------------------------------------------------------------
// XNode
//----------------------------------------------------------------------
const XAttr* XNode::attr(const std::wstring& n) const
{
    for (const auto& a : attrs) if (a.name == n) return &a;
    return nullptr;
}
XAttr* XNode::attr(const std::wstring& n)
{
    for (auto& a : attrs) if (a.name == n) return &a;
    return nullptr;
}
std::wstring XNode::attrValue(const std::wstring& n, const std::wstring& def) const
{
    const XAttr* a = attr(n);
    return a ? a->value : def;
}
long XNode::attrInt(const std::wstring& n, long def) const
{
    const XAttr* a = attr(n);
    if (!a) return def;
    std::wstring v = XTrim(a->value);
    if (v.empty()) return def;
    wchar_t* end = nullptr;
    long r = wcstol(v.c_str(), &end, 10);
    return (end && *end == L'\0') ? r : def;
}
void XNode::setAttr(const std::wstring& n, const std::wstring& v)
{
    XAttr* a = attr(n);
    if (a) a->value = v;
    else attrs.push_back({ n, v });
}
bool XNode::removeAttr(const std::wstring& n)
{
    for (size_t i = 0; i < attrs.size(); i++)
        if (attrs[i].name == n) { attrs.erase(attrs.begin() + (ptrdiff_t)i); return true; }
    return false;
}
const XNode* XNode::child(const std::wstring& n) const
{
    for (const auto& c : children) if (c->name == n) return c.get();
    return nullptr;
}
XNode* XNode::child(const std::wstring& n)
{
    for (auto& c : children) if (c->name == n) return c.get();
    return nullptr;
}
XNode* XNode::addChild(const std::wstring& n)
{
    auto c = std::make_unique<XNode>();
    c->name = n;
    c->parent = this;
    children.push_back(std::move(c));
    return children.back().get();
}
XNode* XNode::insertChild(size_t at, std::unique_ptr<XNode> c)
{
    if (at > children.size()) at = children.size();
    c->parent = this;
    XNode* raw = c.get();
    children.insert(children.begin() + (ptrdiff_t)at, std::move(c));
    return raw;
}
std::unique_ptr<XNode> XNode::removeChild(size_t at)
{
    if (at >= children.size()) return nullptr;
    std::unique_ptr<XNode> c = std::move(children[at]);
    children.erase(children.begin() + (ptrdiff_t)at);
    c->parent = nullptr;
    return c;
}
size_t XNode::indexOf(const XNode* c) const
{
    for (size_t i = 0; i < children.size(); i++) if (children[i].get() == c) return i;
    return SIZE_MAX;
}
std::unique_ptr<XNode> XNode::clone() const
{
    auto c = std::make_unique<XNode>();
    c->name  = name;
    c->attrs = attrs;
    c->text  = text;
    for (const auto& ch : children)
    {
        auto cc = ch->clone();
        cc->parent = c.get();
        c->children.push_back(std::move(cc));
    }
    return c;
}

//----------------------------------------------------------------------
// text helpers
//----------------------------------------------------------------------
std::wstring XTrim(const std::wstring& s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == L' ' || s[a] == L'\t' || s[a] == L'\r' || s[a] == L'\n')) a++;
    while (b > a && (s[b - 1] == L' ' || s[b - 1] == L'\t' || s[b - 1] == L'\r' || s[b - 1] == L'\n')) b--;
    return s.substr(a, b - a);
}

std::wstring XOneLine(const std::wstring& s, size_t maxLen)
{
    std::wstring o;
    bool space = false;
    for (wchar_t ch : s)
    {
        if (ch == L'\r' || ch == L'\n' || ch == L'\t' || ch == L' ')
        {
            if (!space && !o.empty()) o += L' ';
            space = true;
        }
        else { o += ch; space = false; }
    }
    if (maxLen && o.size() > maxLen) { o.resize(maxLen - 1); o += L'\x2026'; }
    return o;
}

std::wstring XEscape(const std::wstring& s, bool attribute)
{
    std::wstring o;
    o.reserve(s.size());
    for (wchar_t ch : s)
    {
        switch (ch)
        {
            case L'&': o += L"&amp;"; break;
            case L'<': o += L"&lt;"; break;
            case L'>': o += L"&gt;"; break;
            case L'"': if (attribute) o += L"&quot;"; else o += ch; break;
            default:   o += ch;
        }
    }
    return o;
}

bool XIsValidUtf8(const uint8_t* p, size_t n)
{
    size_t i = 0;
    while (i < n)
    {
        uint8_t c = p[i];
        size_t k;                                   // continuation bytes expected
        if (c < 0x80) k = 0;
        else if ((c & 0xE0) == 0xC0) k = 1;
        else if ((c & 0xF0) == 0xE0) k = 2;
        else if ((c & 0xF8) == 0xF0) k = 3;
        else return false;
        if (i + k >= n) return false;
        for (size_t j = 1; j <= k; j++) if ((p[i + j] & 0xC0) != 0x80) return false;
        i += k + 1;
    }
    return true;
}

std::wstring XBytesToWide(const uint8_t* p, size_t n, XEncoding enc)
{
    if (n == 0) return L"";
    UINT cp = enc == XENC_UTF8 ? CP_UTF8 : 949;
    int len = MultiByteToWideChar(cp, 0, (const char*)p, (int)n, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring w((size_t)len, L'\0');
    MultiByteToWideChar(cp, 0, (const char*)p, (int)n, &w[0], len);
    return w;
}

bool XWideToBytes(const std::wstring& w, XEncoding enc, std::string& out, std::wstring* badChars)
{
    out.clear();
    if (w.empty()) return true;
    if (enc == XENC_UTF8)
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        out.resize((size_t)len);
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], len, nullptr, nullptr);
        return true;
    }
    BOOL bad = FALSE;
    int len = WideCharToMultiByte(949, WC_NO_BEST_FIT_CHARS, w.c_str(), (int)w.size(), nullptr, 0, nullptr, &bad);
    if (len <= 0 || bad)
    {
        if (badChars)
        {
            badChars->clear();
            for (wchar_t ch : w)
            {
                BOOL b1 = FALSE;
                WideCharToMultiByte(949, WC_NO_BEST_FIT_CHARS, &ch, 1, nullptr, 0, nullptr, &b1);
                if (b1 && badChars->find(ch) == std::wstring::npos) *badChars += ch;
            }
        }
        return false;
    }
    out.resize((size_t)len);
    WideCharToMultiByte(949, WC_NO_BEST_FIT_CHARS, w.c_str(), (int)w.size(), &out[0], len, nullptr, nullptr);
    return true;
}

//----------------------------------------------------------------------
// parser
//----------------------------------------------------------------------
namespace {

struct Parser
{
    const std::wstring& s;
    size_t i = 0;
    XDoc& doc;
    std::wstring err;
    size_t lineAt(size_t pos) const
    {
        size_t line = 1;
        for (size_t k = 0; k < pos && k < s.size(); k++) if (s[k] == L'\n') line++;
        return line;
    }
    bool fail(const std::wstring& what, size_t pos)
    {
        err = what + L" (line " + std::to_wstring(lineAt(pos)) + L")";
        return false;
    }
    static bool isSpace(wchar_t c) { return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n'; }
    static bool isNameChar(wchar_t c) { return iswalnum(c) || c == L'_' || c == L'-' || c == L'.' || c == L':' || c > 127; }
    void skipSpace() { while (i < s.size() && isSpace(s[i])) i++; }
    bool starts(const wchar_t* t) const { return s.compare(i, wcslen(t), t) == 0; }

    static std::wstring decodeEntities(const std::wstring& t)
    {
        if (t.find(L'&') == std::wstring::npos) return t;
        std::wstring o;
        for (size_t k = 0; k < t.size(); k++)
        {
            if (t[k] != L'&') { o += t[k]; continue; }
            size_t semi = t.find(L';', k);
            if (semi == std::wstring::npos || semi - k > 10) { o += t[k]; continue; }
            std::wstring e = t.substr(k + 1, semi - k - 1);
            if (e == L"amp") o += L'&';
            else if (e == L"lt") o += L'<';
            else if (e == L"gt") o += L'>';
            else if (e == L"quot") o += L'"';
            else if (e == L"apos") o += L'\'';
            else if (!e.empty() && e[0] == L'#')
            {
                wchar_t* end = nullptr;
                unsigned long v = e.size() > 1 && (e[1] == L'x' || e[1] == L'X') ? wcstoul(e.c_str() + 2, &end, 16) : wcstoul(e.c_str() + 1, &end, 10);
                if (!end || *end != L'\0' || v == 0 || v > 0xFFFF) { o += t[k]; continue; }
                o += (wchar_t)v;
            }
            else { o += t[k]; continue; }
            k = semi;
        }
        return o;
    }

    bool parseElement(XNode* node)
    {
        // at '<'
        size_t start = i;
        i++;
        size_t ns = i;
        while (i < s.size() && isNameChar(s[i])) i++;
        if (i == ns) return fail(L"Expected an element name after '<'", start);
        node->name = s.substr(ns, i - ns);
        // attributes
        while (true)
        {
            skipSpace();
            if (i >= s.size()) return fail(L"Unterminated start tag <" + node->name, start);
            if (s[i] == L'>') { i++; break; }
            if (s[i] == L'/')
            {
                if (i + 1 < s.size() && s[i + 1] == L'>') { i += 2; return true; }
                return fail(L"Stray '/' in start tag <" + node->name, i);
            }
            size_t as = i;
            while (i < s.size() && isNameChar(s[i])) i++;
            if (i == as) return fail(L"Bad character in start tag <" + node->name, i);
            XAttr a;
            a.name = s.substr(as, i - as);
            skipSpace();
            if (i >= s.size() || s[i] != L'=') return fail(L"Attribute " + a.name + L" of <" + node->name + L"> has no '=' value", as);
            i++;
            skipSpace();
            if (i >= s.size() || (s[i] != L'"' && s[i] != L'\'')) return fail(L"Attribute " + a.name + L" of <" + node->name + L"> is not quoted", as);
            wchar_t q = s[i++];
            size_t vs = i;
            while (i < s.size() && s[i] != q) i++;
            if (i >= s.size()) return fail(L"Unterminated attribute value in <" + node->name, as);
            a.value = decodeEntities(s.substr(vs, i - vs));
            i++;
            node->attrs.push_back(a);
        }
        // content
        std::wstring text;
        while (true)
        {
            size_t ts = i;
            size_t lt = s.find(L'<', i);
            if (lt == std::wstring::npos) return fail(L"Missing </" + node->name + L">", start);
            text += s.substr(ts, lt - ts);
            i = lt;
            if (starts(L"</"))
            {
                size_t cs = i + 2;
                size_t ce = s.find(L'>', cs);
                if (ce == std::wstring::npos) return fail(L"Unterminated end tag", i);
                std::wstring cname = XTrim(s.substr(cs, ce - cs));
                if (cname != node->name) return fail(L"</" + cname + L"> closes <" + node->name + L">", i);
                i = ce + 1;
                break;
            }
            if (starts(L"<!--"))
            {
                size_t ce = s.find(L"-->", i + 4);
                if (ce == std::wstring::npos) return fail(L"Unterminated comment", i);
                doc.warnings.push_back(L"A comment inside <" + node->name + L"> was dropped (line " + std::to_wstring(lineAt(i)) + L"); the client's XML reader cannot handle comments.");
                i = ce + 3;
                continue;
            }
            if (starts(L"<![CDATA["))
            {
                size_t ce = s.find(L"]]>", i + 9);
                if (ce == std::wstring::npos) return fail(L"Unterminated CDATA section", i);
                text += s.substr(i + 9, ce - i - 9);
                i = ce + 3;
                continue;
            }
            if (starts(L"<?"))
            {
                size_t ce = s.find(L"?>", i + 2);
                if (ce == std::wstring::npos) return fail(L"Unterminated processing instruction", i);
                i = ce + 2;
                continue;
            }
            XNode* c = node->addChild(L"");
            if (!parseElement(c)) return false;
        }
        // like the server (xerces handler): every text segment trimmed and concatenated
        node->text = XTrim(decodeEntities(text));
        if (!node->children.empty() && !node->text.empty())
        {
            // text split around children: keep the trimmed whole, which is what both readers end up with
        }
        return true;
    }

    bool parseDoc()
    {
        skipSpace();
        if (i < s.size() && s[i] == L'\xFEFF') i++;
        skipSpace();
        if (starts(L"<?xml"))
        {
            size_t e = s.find(L"?>", i);
            if (e == std::wstring::npos) return fail(L"Unterminated <?xml declaration", i);
            doc.declaration = s.substr(i, e + 2 - i);
            i = e + 2;
        }
        while (true)
        {
            skipSpace();
            if (i >= s.size()) return fail(L"No root element found", 0);
            if (starts(L"<!--"))
            {
                size_t ce = s.find(L"-->", i + 4);
                if (ce == std::wstring::npos) return fail(L"Unterminated comment", i);
                doc.warnings.push_back(L"A comment before the root element was dropped.");
                i = ce + 3;
                continue;
            }
            if (starts(L"<!") || starts(L"<?"))
            {
                size_t ce = s.find(L'>', i);
                if (ce == std::wstring::npos) return fail(L"Unterminated declaration", i);
                i = ce + 1;
                continue;
            }
            if (s[i] != L'<') return fail(L"Text outside the root element", i);
            break;
        }
        doc.root = std::make_unique<XNode>();
        if (!parseElement(doc.root.get())) return false;
        skipSpace();
        while (i < s.size())
        {
            if (starts(L"<!--"))
            {
                size_t ce = s.find(L"-->", i + 4);
                if (ce == std::wstring::npos) break;
                doc.warnings.push_back(L"A comment after the root element was dropped.");
                i = ce + 3;
                skipSpace();
                continue;
            }
            doc.warnings.push_back(L"Content after the root element was ignored (line " + std::to_wstring(lineAt(i)) + L").");
            break;
        }
        return true;
    }
};

std::wstring declaredEncoding(const std::vector<uint8_t>& b)
{
    // look at the ASCII prolog only
    size_t n = b.size() < 200 ? b.size() : 200;
    std::string head((const char*)b.data(), n);
    size_t p = head.find("encoding=");
    if (p == std::string::npos) return L"";
    p += 9;
    if (p >= head.size()) return L"";
    char q = head[p];
    if (q != '"' && q != '\'') return L"";
    size_t e = head.find(q, p + 1);
    if (e == std::string::npos) return L"";
    std::string enc = head.substr(p + 1, e - p - 1);
    for (auto& ch : enc) ch = (char)tolower((unsigned char)ch);
    return std::wstring(enc.begin(), enc.end());
}

} // namespace

bool XDoc::parse(const std::vector<uint8_t>& bytes, std::wstring& err)
{
    root.reset();
    warnings.clear();
    declaration.clear();
    size_t off = 0;
    bom = bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF;
    if (bom) off = 3;
    std::wstring decl = declaredEncoding(bytes);
    bool korean = decl == L"euc-kr" || decl == L"cp949" || decl == L"ks_c_5601-1987" || decl == L"euckr";
    if (bom) encoding = XENC_UTF8;
    else if (korean) encoding = XENC_CP949;
    else encoding = XIsValidUtf8(bytes.data() + off, bytes.size() - off) ? XENC_UTF8 : XENC_CP949;
    if (korean && encoding == XENC_UTF8 && !bom)
        warnings.push_back(L"The file says encoding=\"" + decl + L"\" but its bytes are valid UTF-8; it was read as UTF-8.");

    crlf = false;
    for (size_t k = off; k < bytes.size(); k++) if (bytes[k] == '\r') { crlf = true; break; }

    std::wstring text = XBytesToWide(bytes.data() + off, bytes.size() - off, encoding);
    Parser p{ text, 0, *this };
    if (!p.parseDoc()) { err = p.err; root.reset(); return false; }
    return true;
}

namespace {

void writeNode(const XNode& n, std::wstring& o, int depth, bool clientStyle, const std::wstring& nl)
{
    std::wstring ind((size_t)depth, L'\t');
    o += ind + L"<" + n.name;
    for (const auto& a : n.attrs)
        o += L" " + a.name + L"=\"" + (clientStyle ? a.value : XEscape(a.value, true)) + L"\"";
    bool section = n.name == L"Happen" || n.name == L"Complete" || n.name == L"Fail" || n.name == L"Reward" || n.name == L"QuestList" || n.name == L"Quest";
    std::wstring text = clientStyle ? n.text : XEscape(n.text, false);
    // multi-line text keeps the file's newline style
    if (text.find(L'\n') != std::wstring::npos)
    {
        std::wstring t;
        for (size_t k = 0; k < text.size(); k++)
        {
            if (text[k] == L'\r') continue;
            if (text[k] == L'\n') t += nl; else t += text[k];
        }
        text = t;
    }
    if (n.children.empty())
    {
        if (text.empty())
        {
            if (section) o += L">" + nl + ind + L"</" + n.name + L">" + nl;
            else o += L" />" + nl;
        }
        else o += L">" + text + L"</" + n.name + L">" + nl;
        return;
    }
    o += L">" + text + nl;
    for (const auto& c : n.children) writeNode(*c, o, depth + 1, clientStyle, nl);
    o += ind + L"</" + n.name + L">" + nl;
}

} // namespace

std::wstring XDoc::writeText(bool clientStyle) const
{
    std::wstring nl = crlf ? L"\r\n" : L"\n";
    std::wstring o = encoding == XENC_UTF8 ? L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" : L"<?xml version=\"1.0\" encoding=\"euc-kr\"?>";
    o += nl;
    if (root) writeNode(*root, o, 0, clientStyle, nl);
    return o;
}

void XDoc::write(std::vector<uint8_t>& out, bool clientStyle) const
{
    out.clear();
    std::string bytes;
    XWideToBytes(writeText(clientStyle), encoding, bytes);
    if (bom && encoding == XENC_UTF8) out.assign({ 0xEF, 0xBB, 0xBF });
    out.insert(out.end(), bytes.begin(), bytes.end());
}

std::unique_ptr<XDoc> XDoc::clone() const
{
    auto d = std::make_unique<XDoc>();
    d->encoding = encoding;
    d->bom = bom;
    d->crlf = crlf;
    d->declaration = declaration;
    d->warnings = warnings;
    if (root) d->root = root->clone();
    return d;
}
