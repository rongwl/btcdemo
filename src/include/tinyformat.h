// see https://github.com/bitcoin/bitcoin/blob/master/src/tinyformat.h

#ifndef BTCDEMO_TINYFORMAT_H
#define BTCDEMO_TINYFORMAT_H

namespace tinyformat {}
//------------------------------------------------------------------------------
// Config section.  Customize to your liking!

// Namespace alias to encourage brevity
namespace tfm = tinyformat;

// Error handling; calls assert() by default.
#define TINYFORMAT_ERROR(reasonString) throw std::runtime_error(reasonString)


//------------------------------------------------------------------------------
// Implementation details.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>


namespace tinyformat {

//------------------------------------------------------------------------------
namespace detail {

// Test whether type T1 is convertible to type T2
template <typename T1, typename T2>
struct is_convertible
{
    private:
        // two types of different size
        struct fail { char dummy[2]; };
        struct succeed { char dummy; };
        // Try to convert a T1 to a T2 by plugging into tryConvert
        static fail tryConvert(...);
        static succeed tryConvert(const T2&);
        static const T1& makeT1();
    public:
        // Standard trick: the (...) version of tryConvert will be chosen from
        // the overload set only if the version taking a T2 doesn't match.
        // Then we compare the sizes of the return types to check which
        // function matched.  Very neat, in a disgusting kind of way :)
        static const bool value =
            sizeof(tryConvert(makeT1())) == sizeof(succeed);
};


// Detect when a type is not a wchar_t string
template<typename T> struct is_wchar { typedef int tinyformat_wchar_is_not_supported; };
template<> struct is_wchar<wchar_t*> {};
template<> struct is_wchar<const wchar_t*> {};
template<int n> struct is_wchar<const wchar_t[n]> {};
template<int n> struct is_wchar<wchar_t[n]> {};


// Format the value by casting to type fmtT.  This default implementation
// should never be called.
template<typename T, typename fmtT, bool convertible = is_convertible<T, fmtT>::value>
struct formatValueAsType
{
    static void invoke(std::ostream& /*out*/, const T& /*value*/) { assert(0); }
};
// Specialized version for types that can actually be converted to fmtT, as
// indicated by the "convertible" template parameter.
template<typename T, typename fmtT>
struct formatValueAsType<T,fmtT,true>
{
    static void invoke(std::ostream& out, const T& value)
        { out << static_cast<fmtT>(value); }
};

// Convert an arbitrary type to integer.  The version with convertible=false
// throws an error.
template<typename T, bool convertible = is_convertible<T,int>::value>
struct convertToInt
{
    static int invoke(const T& /*value*/)
    {
        TINYFORMAT_ERROR("tinyformat: Cannot convert from argument type to "
                         "integer for use as variable width or precision");
        return 0;
    }
};
// Specialization for convertToInt when conversion is possible
template<typename T>
struct convertToInt<T,true>
{
    static int invoke(const T& value) { return static_cast<int>(value); }
};

// Format at most ntrunc characters to the given stream.
template<typename T>
inline void formatTruncated(std::ostream& out, const T& value, int ntrunc)
{
    std::ostringstream tmp;
    tmp << value;
    std::string result = tmp.str();
    out.write(result.c_str(), (std::min)(ntrunc, static_cast<int>(result.size())));
}
#define TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR(type)       \
inline void formatTruncated(std::ostream& out, type* value, int ntrunc) \
{                                                           \
    std::streamsize len = 0;                                \
    while(len < ntrunc && value[len] != 0)                  \
        ++len;                                              \
    out.write(value, len);                                  \
}
// Overload for const char* and char*.  Could overload for signed & unsigned
// char too, but these are technically unneeded for printf compatibility.
TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR(const char)
TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR(char)
#undef TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR

} // namespace detail


//------------------------------------------------------------------------------
// Variable formatting functions.  May be overridden for user-defined types if
// desired.


/// Format a value into a stream, delegating to operator<< by default.
///
/// Users may override this for their own types.  When this function is called,
/// the stream flags will have been modified according to the format string.
/// The format specification is provided in the range [fmtBegin, fmtEnd).  For
/// truncating conversions, ntrunc is set to the desired maximum number of
/// characters, for example "%.7s" calls formatValue with ntrunc = 7.
///
/// By default, formatValue() uses the usual stream insertion operator
/// operator<< to format the type T, with special cases for the %c and %p
/// conversions.
template<typename T>
inline void formatValue(std::ostream& out, const char* /*fmtBegin*/,
                        const char* fmtEnd, int ntrunc, const T& value)
{
#ifndef TINYFORMAT_ALLOW_WCHAR_STRINGS
    // Since we don't support printing of wchar_t using "%ls", make it fail at
    // compile time in preference to printing as a void* at runtime.
    typedef typename detail::is_wchar<T>::tinyformat_wchar_is_not_supported DummyType;
    (void) DummyType(); // avoid unused type warning with gcc-4.8
#endif
    // The mess here is to support the %c and %p conversions: if these
    // conversions are active we try to convert the type to a char or const
    // void* respectively and format that instead of the value itself.  For the
    // %p conversion it's important to avoid dereferencing the pointer, which
    // could otherwise lead to a crash when printing a dangling (const char*).
    const bool canConvertToChar = detail::is_convertible<T,char>::value;
    const bool canConvertToVoidPtr = detail::is_convertible<T, const void*>::value;
    if(canConvertToChar && *(fmtEnd-1) == 'c')
        detail::formatValueAsType<T, char>::invoke(out, value);
    else if(canConvertToVoidPtr && *(fmtEnd-1) == 'p')
        detail::formatValueAsType<T, const void*>::invoke(out, value);
    else if(ntrunc >= 0)
    {
        // Take care not to overread C strings in truncating conversions like
        // "%.4s" where at most 4 characters may be read.
        detail::formatTruncated(out, value, ntrunc);
    }
    else
        out << value;
}


// Overloaded version for char types to support printing as an integer
#define TINYFORMAT_DEFINE_FORMATVALUE_CHAR(charType)                  \
inline void formatValue(std::ostream& out, const char* /*fmtBegin*/,  \
                        const char* fmtEnd, int /**/, charType value) \
{                                                                     \
    switch(*(fmtEnd-1))                                               \
    {                                                                 \
        case 'u': case 'd': case 'i': case 'o': case 'X': case 'x':   \
            out << static_cast<int>(value); break;                    \
        default:                                                      \
            out << value;                   break;                    \
    }                                                                 \
}
// per 3.9.1: char, signed char and unsigned char are all distinct types
TINYFORMAT_DEFINE_FORMATVALUE_CHAR(char)
TINYFORMAT_DEFINE_FORMATVALUE_CHAR(signed char)
TINYFORMAT_DEFINE_FORMATVALUE_CHAR(unsigned char)
#undef TINYFORMAT_DEFINE_FORMATVALUE_CHAR


//------------------------------------------------------------------------------
// Tools for emulating variadic templates in C++98.  The basic idea here is
// stolen from the boost preprocessor metaprogramming library and cut down to
// be just general enough for what we need.

#define TINYFORMAT_ARGTYPES(n) TINYFORMAT_ARGTYPES_ ## n
#define TINYFORMAT_VARARGS(n) TINYFORMAT_VARARGS_ ## n
#define TINYFORMAT_PASSARGS(n) TINYFORMAT_PASSARGS_ ## n
#define TINYFORMAT_PASSARGS_TAIL(n) TINYFORMAT_PASSARGS_TAIL_ ## n



namespace detail {

// Type-opaque holder for an argument to format(), with associated actions on
// the type held as explicit function pointers.  This allows FormatArg's for
// each argument to be allocated as a homogenous array inside FormatList
// whereas a naive implementation based on inheritance does not.
class FormatArg
{
    public:
        FormatArg() {}

        template<typename T>
        FormatArg(const T& value)
            : m_value(static_cast<const void*>(&value)),
            m_formatImpl(&formatImpl<T>),
            m_toIntImpl(&toIntImpl<T>)
        { }

        void format(std::ostream& out, const char* fmtBegin,
                    const char* fmtEnd, int ntrunc) const
        {
            m_formatImpl(out, fmtBegin, fmtEnd, ntrunc, m_value);
        }

        int toInt() const
        {
            return m_toIntImpl(m_value);
        }

    private:
        template<typename T>
        static void formatImpl(std::ostream& out, const char* fmtBegin,
                        const char* fmtEnd, int ntrunc, const void* value)
        {
            formatValue(out, fmtBegin, fmtEnd, ntrunc, *static_cast<const T*>(value));
        }

        template<typename T>
        static int toIntImpl(const void* value)
        {
            return convertToInt<T>::invoke(*static_cast<const T*>(value));
        }

        const void* m_value;
        void (*m_formatImpl)(std::ostream& out, const char* fmtBegin,
                             const char* fmtEnd, int ntrunc, const void* value);
        int (*m_toIntImpl)(const void* value);
};


// Parse and return an integer from the string c, as atoi()
// On return, c is set to one past the end of the integer.
inline int parseIntAndAdvance(const char*& c)
{
    int i = 0;
    for(;*c >= '0' && *c <= '9'; ++c)
        i = 10*i + (*c - '0');
    return i;
}

// Print literal part of format string and return next format spec
// position.
//
// Skips over any occurrences of '%%', printing a literal '%' to the
// output.  The position of the first % character of the next
// nontrivial format spec is returned, or the end of string.
inline const char* printFormatStringLiteral(std::ostream& out, const char* fmt)
{
    const char* c = fmt;
    for(;; ++c)
    {
        switch(*c)
        {
            case '\0':
                out.write(fmt, c - fmt);
                return c;
            case '%':
                out.write(fmt, c - fmt);
                if(*(c+1) != '%')
                    return c;
                // for "%%", tack trailing % onto next literal section.
                fmt = ++c;
                break;
            default:
                break;
        }
    }
}


// Parse a format string and set the stream state accordingly.
//
// The format mini-language recognized here is meant to be the one from C99,
// with the form "%[flags][width][.precision][length]type".
//
// Formatting options which can't be natively represented using the ostream
// state are returned in spacePadPositive (for space padded positive numbers)
// and ntrunc (for truncating conversions).  argIndex is incremented if
// necessary to pull out variable width and precision .  The function returns a
// pointer to the character after the end of the current format spec.
inline const char* streamStateFromFormat(std::ostream& out, bool& spacePadPositive,
                                         int& ntrunc, const char* fmtStart,
                                         const detail::FormatArg* formatters,
                                         int& argIndex, int numFormatters)
{
    if(*fmtStart != '%')
    {
        TINYFORMAT_ERROR("tinyformat: Not enough conversion specifiers in format string");
        return fmtStart;
    }
    // Reset stream state to defaults.
    out.width(0);
    out.precision(6);
    out.fill(' ');
    // Reset most flags; ignore irrelevant unitbuf & skipws.
    out.unsetf(std::ios::adjustfield | std::ios::basefield |
               std::ios::floatfield | std::ios::showbase | std::ios::boolalpha |
               std::ios::showpoint | std::ios::showpos | std::ios::uppercase);
    bool precisionSet = false;
    bool widthSet = false;
    int widthExtra = 0;
    const char* c = fmtStart + 1;
    // 1) Parse flags
    for(;; ++c)
    {
        switch(*c)
        {
            case '#':
                out.setf(std::ios::showpoint | std::ios::showbase);
                continue;
            case '0':
                // overridden by left alignment ('-' flag)
                if(!(out.flags() & std::ios::left))
                {
                    // Use internal padding so that numeric values are
                    // formatted correctly, eg -00010 rather than 000-10
                    out.fill('0');
                    out.setf(std::ios::internal, std::ios::adjustfield);
                }
                continue;
            case '-':
                out.fill(' ');
                out.setf(std::ios::left, std::ios::adjustfield);
                continue;
            case ' ':
                // overridden by show positive sign, '+' flag.
                if(!(out.flags() & std::ios::showpos))
                    spacePadPositive = true;
                continue;
            case '+':
                out.setf(std::ios::showpos);
                spacePadPositive = false;
                widthExtra = 1;
                continue;
            default:
                break;
        }
        break;
    }
    // 2) Parse width
    if(*c >= '0' && *c <= '9')
    {
        widthSet = true;
        out.width(parseIntAndAdvance(c));
    }
    if(*c == '*')
    {
        widthSet = true;
        int width = 0;
        if(argIndex < numFormatters)
            width = formatters[argIndex++].toInt();
        else
            TINYFORMAT_ERROR("tinyformat: Not enough arguments to read variable width");
        if(width < 0)
        {
            // negative widths correspond to '-' flag set
            out.fill(' ');
            out.setf(std::ios::left, std::ios::adjustfield);
            width = -width;
        }
        out.width(width);
        ++c;
    }
    // 3) Parse precision
    if(*c == '.')
    {
        ++c;
        int precision = 0;
        if(*c == '*')
        {
            ++c;
            if(argIndex < numFormatters)
                precision = formatters[argIndex++].toInt();
            else
                TINYFORMAT_ERROR("tinyformat: Not enough arguments to read variable precision");
        }
        else
        {
            if(*c >= '0' && *c <= '9')
                precision = parseIntAndAdvance(c);
            else if(*c == '-') // negative precisions ignored, treated as zero.
                parseIntAndAdvance(++c);
        }
        out.precision(precision);
        precisionSet = true;
    }
    // 4) Ignore any C99 length modifier
    while(*c == 'l' || *c == 'h' || *c == 'L' ||
          *c == 'j' || *c == 'z' || *c == 't')
        ++c;
    // 5) We're up to the conversion specifier character.
    // Set stream flags based on conversion specifier (thanks to the
    // boost::format class for forging the way here).
    bool intConversion = false;
    switch(*c)
    {
        case 'u': case 'd': case 'i':
            out.setf(std::ios::dec, std::ios::basefield);
            intConversion = true;
            break;
        case 'o':
            out.setf(std::ios::oct, std::ios::basefield);
            intConversion = true;
            break;
        case 'X':
            out.setf(std::ios::uppercase);
        case 'x': case 'p':
            out.setf(std::ios::hex, std::ios::basefield);
            intConversion = true;
            break;
        case 'E':
            out.setf(std::ios::uppercase);
        case 'e':
            out.setf(std::ios::scientific, std::ios::floatfield);
            out.setf(std::ios::dec, std::ios::basefield);
            break;
        case 'F':
            out.setf(std::ios::uppercase);
        case 'f':
            out.setf(std::ios::fixed, std::ios::floatfield);
            break;
        case 'G':
            out.setf(std::ios::uppercase);
        case 'g':
            out.setf(std::ios::dec, std::ios::basefield);
            // As in boost::format, let stream decide float format.
            out.flags(out.flags() & ~std::ios::floatfield);
            break;
        case 'a': case 'A':
            TINYFORMAT_ERROR("tinyformat: the %a and %A conversion specs "
                             "are not supported");
            break;
        case 'c':
            // Handled as special case inside formatValue()
            break;
        case 's':
            if(precisionSet)
                ntrunc = static_cast<int>(out.precision());
            // Make %s print booleans as "true" and "false"
            out.setf(std::ios::boolalpha);
            break;
        case 'n':
            // Not supported - will cause problems!
            TINYFORMAT_ERROR("tinyformat: %n conversion spec not supported");
            break;
        case '\0':
            TINYFORMAT_ERROR("tinyformat: Conversion spec incorrectly "
                             "terminated by end of string");
            return c;
        default:
            break;
    }
    if(intConversion && precisionSet && !widthSet)
    {
        // "precision" for integers gives the minimum number of digits (to be
        // padded with zeros on the left).  This isn't really supported by the
        // iostreams, but we can approximately simulate it with the width if
        // the width isn't otherwise used.
        out.width(out.precision() + widthExtra);
        out.setf(std::ios::internal, std::ios::adjustfield);
        out.fill('0');
    }
    return c+1;
}


//------------------------------------------------------------------------------
inline void formatImpl(std::ostream& out, const char* fmt,
                       const detail::FormatArg* formatters,
                       int numFormatters)
{
    // Saved stream state
    std::streamsize origWidth = out.width();
    std::streamsize origPrecision = out.precision();
    std::ios::fmtflags origFlags = out.flags();
    char origFill = out.fill();

    for (int argIndex = 0; argIndex < numFormatters; ++argIndex)
    {
        // Parse the format string
        fmt = printFormatStringLiteral(out, fmt);
        bool spacePadPositive = false;
        int ntrunc = -1;
        const char* fmtEnd = streamStateFromFormat(out, spacePadPositive, ntrunc, fmt,
                                                   formatters, argIndex, numFormatters);
        if (argIndex >= numFormatters)
        {
            // Check args remain after reading any variable width/precision
            TINYFORMAT_ERROR("tinyformat: Not enough format arguments");
            return;
        }
        const FormatArg& arg = formatters[argIndex];
        // Format the arg into the stream.
        if(!spacePadPositive)
            arg.format(out, fmt, fmtEnd, ntrunc);
        else
        {
            // The following is a special case with no direct correspondence
            // between stream formatting and the printf() behaviour.  Simulate
            // it crudely by formatting into a temporary string stream and
            // munging the resulting string.
            std::ostringstream tmpStream;
            tmpStream.copyfmt(out);
            tmpStream.setf(std::ios::showpos);
            arg.format(tmpStream, fmt, fmtEnd, ntrunc);
            std::string result = tmpStream.str(); // allocates... yuck.
            for(size_t i = 0, iend = result.size(); i < iend; ++i)
                if(result[i] == '+') result[i] = ' ';
            out << result;
        }
        fmt = fmtEnd;
    }

    // Print remaining part of format string.
    fmt = printFormatStringLiteral(out, fmt);
    if(*fmt != '\0')
        TINYFORMAT_ERROR("tinyformat: Too many conversion specifiers in format string");

    // Restore stream state
    out.width(origWidth);
    out.precision(origPrecision);
    out.flags(origFlags);
    out.fill(origFill);
}

} // namespace detail


/// List of template arguments format(), held in a type-opaque way.
///
/// A const reference to FormatList (typedef'd as FormatListRef) may be
/// conveniently used to pass arguments to non-template functions: All type
/// information has been stripped from the arguments, leaving just enough of a
/// common interface to perform formatting as required.
class FormatList
{
    public:
        FormatList(detail::FormatArg* formatters, int N)
            : m_formatters(formatters), m_N(N) { }

        friend void vformat(std::ostream& out, const char* fmt,
                            const FormatList& list);

    private:
        const detail::FormatArg* m_formatters;
        int m_N;
};

/// Reference to type-opaque format list for passing to vformat()
typedef const FormatList& FormatListRef;


namespace detail {

// Format list subclass with fixed storage to avoid dynamic allocation
template<int N>
class FormatListN : public FormatList
{
    public:
        template<typename... Args>
        FormatListN(const Args&... args)
            : FormatList(&m_formatterStore[0], N),
            m_formatterStore { FormatArg(args)... }
        { static_assert(sizeof...(args) == N, "Number of args must be N"); }
    private:
        FormatArg m_formatterStore[N];
};

// Special 0-arg version - MSVC says zero-sized C array in struct is nonstandard
template<> class FormatListN<0> : public FormatList
{
    public: FormatListN() : FormatList(0, 0) {}
};

} // namespace detail


//------------------------------------------------------------------------------
// Primary API functions

/// Make type-agnostic format list from list of template arguments.
///
/// The exact return type of this function is an implementation detail and
/// shouldn't be relied upon.  Instead it should be stored as a FormatListRef:
///
///   FormatListRef formatList = makeFormatList( /*...*/ );
template<typename... Args>
detail::FormatListN<sizeof...(Args)> makeFormatList(const Args&... args)
{
    return detail::FormatListN<sizeof...(args)>(args...);
}



/// Format list of arguments to the stream according to the given format string.
///
/// The name vformat() is chosen for the semantic similarity to vprintf(): the
/// list of format arguments is held in a single function argument.
inline void vformat(std::ostream& out, const char* fmt, FormatListRef list)
{
    detail::formatImpl(out, fmt, list.m_formatters, list.m_N);
}


/// Format list of arguments to the stream according to given format string.
template<typename... Args>
void format(std::ostream& out, const char* fmt, const Args&... args)
{
    vformat(out, fmt, makeFormatList(args...));
}

/// Format list of arguments according to the given format string and return
/// the result as a string.
template<typename... Args>
std::string format(const char* fmt, const Args&... args)
{
    std::ostringstream oss;
    format(oss, fmt, args...);
    return oss.str();
}

/// Format list of arguments to std::cout, according to the given format string
template<typename... Args>
void printf(const char* fmt, const Args&... args)
{
    format(std::cout, fmt, args...);
}

template<typename... Args>
void printfln(const char* fmt, const Args&... args)
{
    format(std::cout, fmt, args...);
    std::cout << '\n';
}



template<typename... Args>
std::string format(const std::string &fmt, const Args&... args)
{
    std::ostringstream oss;
    format(oss, fmt.c_str(), args...);
    return oss.str();
}

} // namespace tinyformat

#define strprintf tfm::format

#endif // BTCDEMO_TINYFORMAT_H
