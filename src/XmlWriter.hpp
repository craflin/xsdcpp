
#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>

#ifdef __GNUC__
#define XSDCPP_MAYBE_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
#define XSDCPP_MAYBE_UNUSED
#else
#define XSDCPP_MAYBE_UNUSED
#endif

namespace xsdcpp {

class XmlWriter {
public:
    XmlWriter()
        : _depth(0)
        , _tagOpen(false)
        , _hasContent(false)
    {
    }

    void startElement(const char* name)
    {
        closeTagIfOpen();
        _buffer += std::string(_depth * 2, ' ');
        _buffer += '<';
        _buffer += name;
        _tagOpen = true;
        _hasContent = false;
        ++_depth;
    }

    void endElement(const char* name)
    {
        --_depth;
        if (_tagOpen)
        {
            _buffer += "/>\n";
            _tagOpen = false;
        }
        else
        {
            if (!_hasContent)
                _buffer += std::string(_depth * 2, ' ');
            _buffer += "</";
            _buffer += name;
            _buffer += ">\n";
        }
        _hasContent = false;
    }

    void writeAttribute(const char* name, const std::string& value)
    {
        _buffer += ' ';
        _buffer += name;
        _buffer += "=\"";
        _buffer += escape_xml(value);
        _buffer += '"';
    }

    void writeText(const std::string& text)
    {
        // Close tag without newline - text follows immediately
        if (_tagOpen)
        {
            _buffer += ">";
            _tagOpen = false;
        }
        _buffer += escape_xml(text);
        _hasContent = true;
    }

    std::string str() const
    {
        return _buffer;
    }

private:
    void closeTagIfOpen()
    {
        if (_tagOpen)
        {
            _buffer += ">\n";
            _tagOpen = false;
        }
    }

    static std::string escape_xml(const std::string& s)
    {
        std::string result;
        result.reserve(s.size());
        for (char c : s)
        {
            switch (c)
            {
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '&':
                result += "&amp;";
                break;
            case '"':
                result += "&quot;";
                break;
            case '\'':
                result += "&apos;";
                break;
            default:
                result += c;
                break;
            }
        }
        return result;
    }

    std::string _buffer;
    int _depth;
    bool _tagOpen;
    bool _hasContent;
};

inline std::string get_string(const std::string& v) { return v; }
inline std::string get_string(uint64_t v) { return std::to_string(v); }
inline std::string get_string(int64_t v) { return std::to_string(v); }
inline std::string get_string(uint32_t v) { return std::to_string(v); }
inline std::string get_string(int32_t v) { return std::to_string(v); }
inline std::string get_string(uint16_t v) { return std::to_string(v); }
inline std::string get_string(int16_t v) { return std::to_string(v); }
inline std::string get_string(bool v) { return v ? "true" : "false"; }

inline std::string get_string(float v)
{
    std::ostringstream ss;
    ss << std::setprecision(9) << v;
    return ss.str();
}

inline std::string get_string(double v)
{
    std::ostringstream ss;
    ss << std::setprecision(17) << v;
    return ss.str();
}

inline void write_file(const std::string& filePath, const std::string& content)
{
    std::ofstream file;
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    file.open(filePath);
    file << content;
}

template <typename T>
inline std::string _serialize_list(const T& vec)
{
    std::string result;
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if (i > 0) result += ' ';
        result += get_string(vec[i]);
    }
    return result;
}

}
