#include "json.hpp"
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace ws {
namespace {
[[noreturn]] void bad() { throw std::runtime_error("Invalid JSON: malformed, oversized, duplicate key or wrong type"); }
struct Parser {
    std::string_view s;
    std::size_t p = 0;
    void space() { while (p < s.size() && (s[p]==' ' || s[p]=='\n' || s[p]=='\r' || s[p]=='\t')) ++p; }
    bool take(char c) { space(); if (p < s.size() && s[p]==c) { ++p; return true; } return false; }
    void need(char c) { if (!take(c)) bad(); }
    unsigned hex4() {
        unsigned v=0;
        for (int i=0;i<4;++i) {
            if (p==s.size()) bad();
            const char c=s[p++];
            const int d=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
            if (d<0) bad();
            v=v*16+static_cast<unsigned>(d);
        }
        return v;
    }
    std::string str() {
        need('"'); std::string out;
        while (p < s.size()) {
            const unsigned char c=static_cast<unsigned char>(s[p++]);
            if (c=='"') return out;
            if (c<32) bad();
            if (c!='\\') { out+=static_cast<char>(c); continue; }
            if (p==s.size()) bad();
            switch(s[p++]) {
            case '"': out+='"'; break; case '\\': out+='\\'; break; case '/': out+='/'; break;
            case 'b': out+='\b'; break; case 'f': out+='\f'; break; case 'n': out+='\n'; break;
            case 'r': out+='\r'; break; case 't': out+='\t'; break;
            case 'u': {
                unsigned v=hex4();
                if (v>=0xD800 && v<=0xDBFF) {
                    if (p+2>s.size() || s[p]!='\\' || s[p+1]!='u') bad();
                    p+=2; const unsigned low=hex4();
                    if (low<0xDC00 || low>0xDFFF) bad();
                    v=0x10000+(v-0xD800)*1024+low-0xDC00;
                } else if (v>=0xDC00 && v<=0xDFFF) bad();
                if (v<128) out+=static_cast<char>(v);
                else {
                    if (v<2048) out+=static_cast<char>(0xC0|(v>>6));
                    else {
                        if (v<65536) out+=static_cast<char>(0xE0|(v>>12));
                        else { out+=static_cast<char>(0xF0|(v>>18)); out+=static_cast<char>(0x80|((v>>12)&63)); }
                        out+=static_cast<char>(0x80|((v>>6)&63));
                    }
                    out+=static_cast<char>(0x80|(v&63));
                }
                break;
            }
            default: bad();
            }
        }
        bad();
    }
    Json read(int depth) {
        if (depth>24) bad();
        space(); if (p==s.size()) bad();
        if (s[p]=='"') return Json{str()};
        if (take('{')) {
            Json::Object o;
            if (!take('}')) { do {
                const auto k=str(); need(':');
                if (!o.emplace(k,read(depth+1)).second) bad();
            } while(take(',')); need('}'); }
            return Json{std::move(o)};
        }
        if (take('[')) {
            Json::Array a;
            if (!take(']')) { do { a.push_back(read(depth+1)); } while(take(',')); need(']'); }
            return Json{std::move(a)};
        }
        for (const auto word : {std::string_view("true"),std::string_view("false"),std::string_view("null")}) {
            if (s.substr(p,word.size())==word) { p+=word.size(); if(word=="null") return Json{nullptr}; return Json{word=="true"}; }
        }
        const auto start=p;
        if (s[p]=='-') ++p;
        if (p==s.size()) bad();
        if (s[p]=='0') ++p;
        else { if(s[p]<'1'||s[p]>'9') bad(); while(p<s.size()&&s[p]>='0'&&s[p]<='9') ++p; }
        auto digits=[&] { const auto old=p; while(p<s.size()&&s[p]>='0'&&s[p]<='9') ++p; if(old==p) bad(); };
        if (p<s.size()&&s[p]=='.') { ++p; digits(); }
        if (p<s.size()&&(s[p]=='e'||s[p]=='E')) { ++p; if(p<s.size()&&(s[p]=='+'||s[p]=='-')) ++p; digits(); }
        double n=0;
        const auto result=std::from_chars(s.data()+start,s.data()+p,n);
        if(result.ec!=std::errc{} || result.ptr!=s.data()+p || !std::isfinite(n)) bad();
        return Json{n};
    }
};
}
const Json& Json::at(std::string_view key) const {
    const auto* o=std::get_if<Object>(&value); if(!o) bad();
    const auto i=o->find(key); if(i==o->end()) throw std::runtime_error("Fixture missing field: "+std::string(key));
    return i->second;
}
const Json::Array& Json::array() const { const auto* p=std::get_if<Array>(&value); if(!p) bad(); return *p; }
const std::string& Json::string() const { const auto* p=std::get_if<std::string>(&value); if(!p) bad(); return *p; }
double Json::number() const { const auto* p=std::get_if<double>(&value); if(!p) bad(); return *p; }
Json parseJson(std::string_view text) {
    if(text.size()>65536) bad();
    Parser p{text}; auto r=p.read(0); p.space(); if(p.p!=text.size()) bad(); return r;
}
std::string quoteJson(std::string_view text) {
    std::string s="\"";
    constexpr char hex[]="0123456789abcdef";
    for(unsigned char c:text) {
        if(c=='"'||c=='\\') { s+='\\'; s+=static_cast<char>(c); }
        else if(c<32) { s+="\\u00"; s+=hex[c>>4]; s+=hex[c&15]; }
        else s+=static_cast<char>(c);
    }
    return s+'"';
}
}
