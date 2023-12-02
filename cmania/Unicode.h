#pragma once
#include <string>


inline void Ucs4Char2Utf8(int codePoint,char* outbuf,int& outlen)
{
    outlen = 0;
    if (codePoint <= 0x7F)
    {
        // Single byte UTF-8 encoding for ASCII characters
        *outbuf = static_cast<char>(codePoint);
        outlen = 1;
    }
    else if (codePoint <= 0x7FF)
    {
        // Two byte UTF-8 encoding
        *outbuf = static_cast<char>(0xC0 | (codePoint >> 6));             // Byte 1
        outbuf++;
        *outbuf = static_cast<char>(0x80 | (codePoint & 0x3F));           // Byte 2
        outlen = 2;
    }
    else if (codePoint <= 0xFFFF)
    {
        // Three byte UTF-8 encoding
        *outbuf = static_cast<char>(0xE0 | (codePoint >> 12));            // Byte 1
        outbuf++;
        *outbuf = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));    // Byte 2
        outbuf++;
        *outbuf = static_cast<char>(0x80 | (codePoint & 0x3F));           // Byte 3
        outlen = 3;
    }
    else if (codePoint <= 0x10FFFF)
    {
        // Four byte UTF-8 encoding
        *outbuf = static_cast<char>(0xF0 | (codePoint >> 18));            // Byte 1
        outbuf++;
        *outbuf = static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));   // Byte 2
        outbuf++;
        *outbuf = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));    // Byte 3
        outbuf++;
        *outbuf = static_cast<char>(0x80 | (codePoint & 0x3F));           // Byte 4
        outlen = 4;
    }
}
template<class T>
inline std::u32string Utf82Ucs4(const std::basic_string<T>& utf8String)
{
    std::u32string ucs4String;

    for (size_t i = 0; i < utf8String.length();)
    {
        char32_t unicodeValue = 0;
        size_t codeLength = 0;

        // Check the number of leading 1s to determine the length of the UTF-8 code
        if ((utf8String[i] & 0x80) == 0)
        {
            // Single-byte character (ASCII)
            unicodeValue = utf8String[i];
            codeLength = 1;
        }
        else if ((utf8String[i] & 0xE0) == 0xC0)
        {
            // Two-byte character
            unicodeValue = (utf8String[i] & 0x1F) << 6;
            unicodeValue |= (utf8String[i + 1] & 0x3F);
            codeLength = 2;
        }
        else if ((utf8String[i] & 0xF0) == 0xE0)
        {
            // Three-byte character
            unicodeValue = (utf8String[i] & 0x0F) << 12;
            unicodeValue |= (utf8String[i + 1] & 0x3F) << 6;
            unicodeValue |= (utf8String[i + 2] & 0x3F);
            codeLength = 3;
        }
        else if ((utf8String[i] & 0xF8) == 0xF0)
        {
            // Four-byte character
            unicodeValue = (utf8String[i] & 0x07) << 18;
            unicodeValue |= (utf8String[i + 1] & 0x3F) << 12;
            unicodeValue |= (utf8String[i + 2] & 0x3F) << 6;
            unicodeValue |= (utf8String[i + 3] & 0x3F);
            codeLength = 4;
        }
        else
        {
            unicodeValue = '?';
            codeLength = 1;
        }

        ucs4String.push_back(unicodeValue);
        i += codeLength;
    }

    return ucs4String;
}
inline std::string Utf162Utf8(const std::wstring& utf16String) {
    std::string utf8String;

    for (const auto& ch : utf16String) {
        if (ch <= 0x7F) {
            // Single-byte character (ASCII)
            utf8String.push_back(static_cast<char>(ch));
        }
        else if (ch <= 0x07FF) {
            // Two-byte character
            utf8String.push_back(static_cast<char>((ch >> 6) | 0xC0));
            utf8String.push_back(static_cast<char>((ch & 0x3F) | 0x80));
        }
        else {
            // Three-byte character
            utf8String.push_back(static_cast<char>((ch >> 12) | 0xE0));
            utf8String.push_back(static_cast<char>(((ch >> 6) & 0x3F) | 0x80));
            utf8String.push_back(static_cast<char>((ch & 0x3F) | 0x80));
        }
    }

    return utf8String;
}
inline bool IsMultiUtf16(wchar_t chr)
{
    return chr >= 0xD800 && chr <= 0xDBFF;
}
inline bool IsMultiUtf16Tail(wchar_t chr)
{
    return chr >= 0xDC00 && chr <= 0xDFFF;
}
inline std::u32string Utf162Ucs4(const std::wstring& utf16String) {
    std::u32string ucs4String;

    for (size_t i = 0; i < utf16String.length(); ++i) {
        char32_t unicodeValue = utf16String[i];

        // Handle surrogate pairs
        if (unicodeValue >= 0xD800 && unicodeValue <= 0xDBFF && i + 1 < utf16String.length()) {
            char32_t highSurrogate = unicodeValue;
            char32_t lowSurrogate = utf16String[i + 1];

            if (lowSurrogate >= 0xDC00 && lowSurrogate <= 0xDFFF) {
                unicodeValue = ((highSurrogate - 0xD800) << 10) + (lowSurrogate - 0xDC00) + 0x10000;
                ++i; // Skip the low surrogate
            }
        }

        ucs4String.push_back(unicodeValue);
    }

    return ucs4String;
}

inline std::string UCS4ToUTF8(const std::u32string& ucs4String)
{
    std::string utf8String;

    for (char32_t codePoint : ucs4String)
    {
        if (codePoint <= 0x7F)
        {
            // Single-byte character (ASCII)
            utf8String.push_back(static_cast<char>(codePoint));
        }
        else if (codePoint <= 0x7FF)
        {
            // Two-byte character
            utf8String.push_back(static_cast<char>((codePoint >> 6) | 0xC0));
            utf8String.push_back(static_cast<char>((codePoint & 0x3F) | 0x80));
        }
        else if (codePoint <= 0xFFFF)
        {
            // Three-byte character
            utf8String.push_back(static_cast<char>((codePoint >> 12) | 0xE0));
            utf8String.push_back(static_cast<char>(((codePoint >> 6) & 0x3F) | 0x80));
            utf8String.push_back(static_cast<char>((codePoint & 0x3F) | 0x80));
        }
        else if (codePoint <= 0x10FFFF)
        {
            // Four-byte character
            utf8String.push_back(static_cast<char>((codePoint >> 18) | 0xF0));
            utf8String.push_back(static_cast<char>(((codePoint >> 12) & 0x3F) | 0x80));
            utf8String.push_back(static_cast<char>(((codePoint >> 6) & 0x3F) | 0x80));
            utf8String.push_back(static_cast<char>((codePoint & 0x3F) | 0x80));
        }
        else
        {
            utf8String.push_back('?');
        }
    }

    return utf8String;
}