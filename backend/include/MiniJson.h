#ifndef CC_MINI_JSON_H
#define CC_MINI_JSON_H
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace cc { namespace json {
class Value {
public:
 enum Type { Null, Bool, Number, String, Object, Array };
 Type type=Null; bool b=false; double n=0; std::string s; std::map<std::string,Value> o; std::vector<Value> a;
 Value()=default; Value(std::nullptr_t):type(Null){} Value(bool v):type(Bool),b(v){} Value(int v):type(Number),n(v){} Value(double v):type(Number),n(v){} Value(const char* v):type(String),s(v){} Value(const std::string& v):type(String),s(v){}
 static Value object(){Value v;v.type=Object;return v;} static Value array(){Value v;v.type=Array;return v;}
 Value& operator[](const std::string& k){if(type!=Object) {type=Object;o.clear();} return o[k];}
 const Value& at(const std::string& k) const {auto it=o.find(k); if(it==o.end()) throw std::runtime_error("Missing JSON field: "+k); return it->second;}
 bool has(const std::string& k) const {return type==Object && o.find(k)!=o.end();}
 void push(const Value& v){if(type!=Array){type=Array;a.clear();}a.push_back(v);}
 std::string str(const std::string& def="") const {return type==String?s:def;}
 int integer(int def=0) const {return type==Number?(int)n:def;}
 bool boolean(bool def=false) const {return type==Bool?b:def;}
};
inline std::string esc(const std::string& x){std::ostringstream q; for(unsigned char c:x){switch(c){case '"':q<<"\\\"";break;case '\\':q<<"\\\\";break;case '\b':q<<"\\b";break;case '\f':q<<"\\f";break;case '\n':q<<"\\n";break;case '\r':q<<"\\r";break;case '\t':q<<"\\t";break;default: if(c<0x20){q<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<(int)c<<std::dec;}else q<<c;}}return q.str();}
inline std::string dump(const Value& v){std::ostringstream q; switch(v.type){case Value::Null:q<<"null";break;case Value::Bool:q<<(v.b?"true":"false");break;case Value::Number:q<<std::setprecision(15)<<v.n;break;case Value::String:q<<'"'<<esc(v.s)<<'"';break;case Value::Array:q<<'[';for(size_t i=0;i<v.a.size();++i){if(i)q<<',';q<<dump(v.a[i]);}q<<']';break;case Value::Object:q<<'{';{bool first=true;for(const auto& p:v.o){if(!first)q<<',';first=false;q<<'"'<<esc(p.first)<<'"'<<':'<<dump(p.second);}}q<<'}';break;}return q.str();}
class Parser{const std::string& s;size_t i=0;void ws(){while(i<s.size()&&std::isspace((unsigned char)s[i]))++i;}char peek(){ws();return i<s.size()?s[i]:'\0';}void eat(char c){ws();if(i>=s.size()||s[i]!=c)throw std::runtime_error("Invalid JSON");++i;}std::string str(){eat('"');std::string r;while(i<s.size()){char c=s[i++];if(c=='"')return r;if(c=='\\'){if(i>=s.size())throw std::runtime_error("Invalid escape");char e=s[i++];switch(e){case '"':r+='"';break;case '\\':r+='\\';break;case '/':r+='/';break;case 'b':r+='\b';break;case 'f':r+='\f';break;case 'n':r+='\n';break;case 'r':r+='\r';break;case 't':r+='\t';break;default:throw std::runtime_error("Unsupported JSON escape");}}else r+=c;}throw std::runtime_error("Unterminated string");}
Value val(){ws();if(i>=s.size())throw std::runtime_error("Invalid JSON");char c=s[i];if(c=='"')return Value(str());if(c=='{')return obj();if(c=='[')return arr();if(s.compare(i,4,"true")==0){i+=4;return Value(true);}if(s.compare(i,5,"false")==0){i+=5;return Value(false);}if(s.compare(i,4,"null")==0){i+=4;return Value();}char* end=nullptr;double n=std::strtod(s.c_str()+i,&end);if(end==s.c_str()+i)throw std::runtime_error("Invalid JSON value");i=(size_t)(end-s.c_str());return Value(n);}
Value obj(){Value v=Value::object();eat('{');ws();if(peek()=='}'){++i;return v;}while(true){std::string k=str();eat(':');v[k]=val();ws();if(peek()=='}'){++i;break;}eat(',');}return v;}Value arr(){Value v=Value::array();eat('[');ws();if(peek()==']'){++i;return v;}while(true){v.push(val());ws();if(peek()==']'){++i;break;}eat(',');}return v;}
public:Parser(const std::string& x):s(x){}Value parse(){Value v=val();ws();if(i!=s.size())throw std::runtime_error("Trailing JSON data");return v;}};
inline Value parse(const std::string& s){return Parser(s).parse();}
inline std::string getString(const Value& v,const std::string& k,const std::string& d=""){return v.has(k)?v.at(k).str(d):d;}inline int getInt(const Value&v,const std::string&k,int d=0){return v.has(k)?v.at(k).integer(d):d;}inline bool getBool(const Value&v,const std::string&k,bool d=false){return v.has(k)?v.at(k).boolean(d):d;}
inline Value arrStrings(const std::vector<std::string>& xs){Value v=Value::array();for(const auto&x:xs)v.push(x);return v;}
}}
#endif
