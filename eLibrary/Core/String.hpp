#pragma once

#include <algorithm>
#include <codecvt>
#include <cstring>
#include <locale>
#include <map>
#include <string>

#include <Core/Object.hpp>

namespace eLibrary {
    class String final : public Object {
    private:
        intmax_t CharacterSize;
        char16_t *CharacterContainer;
        uintmax_t *CharacterReference;
    public:
        String() noexcept: CharacterSize(0), CharacterContainer(nullptr) {
            CharacterReference = new uintmax_t;
            *CharacterReference = 1;
        }

        explicit String(char16_t CharacterSource) noexcept : CharacterSize(1) {
            CharacterContainer = new char16_t[2];
            CharacterContainer[0] = CharacterSource;
            CharacterContainer[1] = char16_t();
            CharacterReference = new uintmax_t;
            *CharacterReference = 1;
        }

        String(const String &StringSource) noexcept : CharacterSize(StringSource.CharacterSize), CharacterContainer(StringSource.CharacterContainer), CharacterReference(StringSource.CharacterReference) {
            ++(*CharacterReference);
        }

        String(const std::string &StringSource) noexcept : CharacterSize((intmax_t) StringSource.size()) {
            static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> StringConverter;
            std::u16string String16Source = StringConverter.from_bytes(StringSource);
            CharacterContainer = new char16_t[String16Source.size() + 1];
            std::copy(String16Source.begin(), String16Source.end(), CharacterContainer);
            CharacterContainer[CharacterSize] = char16_t();
            CharacterReference = new uintmax_t;
            *CharacterReference = 1;
        }

        String(const std::u16string &StringSource) noexcept : CharacterSize((intmax_t) StringSource.size()) {
            CharacterContainer = new char16_t[StringSource.size() + 1];
            std::copy(StringSource.begin(), StringSource.end(), CharacterContainer);
            CharacterContainer[CharacterSize] = char16_t();
            CharacterReference = new uintmax_t;
            *CharacterReference = 1;
        }

        String(const std::u32string &StringSource) noexcept : CharacterSize((intmax_t) StringSource.size()) {
            static std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t> StringConverter;
            std::string String8Source = StringConverter.to_bytes(StringSource);
            std::u16string String16Source(reinterpret_cast<const char16_t*>(String8Source.c_str()), String8Source.size() / sizeof(char16_t));
            CharacterContainer = new char16_t[String16Source.size() + 1];
            std::copy(String16Source.begin(), String16Source.end(), CharacterContainer);
            CharacterContainer[CharacterSize] = char16_t();
            CharacterReference = new uintmax_t;
            *CharacterReference = 1;
        }

        String(const std::wstring &StringSource) noexcept : CharacterSize((intmax_t) StringSource.size()) {
            CharacterContainer = new char16_t[StringSource.size() + 1];
            std::copy(StringSource.begin(), StringSource.end(), CharacterContainer);
            CharacterContainer[CharacterSize] = char16_t();
            CharacterReference = new uintmax_t;
            *CharacterReference = 1;
        }

        ~String() noexcept {
            if (--(*CharacterReference) == 0) {
                CharacterSize = 0;
                delete[] CharacterContainer;
                CharacterContainer = nullptr;
                delete CharacterReference;
                CharacterReference = nullptr;
            }
        }

        void doAssign(const String &StringSource) noexcept {
            if (&StringSource == this) return;
            delete[] CharacterContainer;
            delete[] CharacterReference;
            CharacterContainer = StringSource.CharacterContainer;
            CharacterSize = StringSource.CharacterSize;
            CharacterReference = StringSource.CharacterReference;
            ++(*CharacterReference);
        }

        intmax_t doCompare(const String &StringOther) const noexcept {
            if (CharacterSize != StringOther.CharacterSize) return CharacterSize - StringOther.CharacterSize;
            for (intmax_t CharacterIndex = 0; CharacterIndex < CharacterSize; ++CharacterIndex)
                if (CharacterContainer[CharacterIndex] != StringOther.CharacterContainer[CharacterIndex])
                    return CharacterContainer[CharacterIndex] - StringOther.CharacterContainer[CharacterIndex];
            return 0;
        }

        String doConcat(char16_t CharacterSource) const noexcept;

        String doConcat(const String &StringOther) const noexcept;

        intmax_t doFind(char16_t CharacterSource) const noexcept {
            for (char16_t CharacterIndex = 0;CharacterIndex < CharacterSize;++CharacterIndex)
                if (CharacterContainer[CharacterIndex] == CharacterSource) return CharacterIndex;
            return -1;
        }

        intmax_t doFind(const String &StringTarget) const noexcept {
            intmax_t Character1, Character2;
            for (Character1 = 0, Character2 = 0;Character1 < CharacterSize && Character2 < StringTarget.CharacterSize; ++Character1)
                if (CharacterContainer[Character1] == StringTarget.CharacterContainer[Character2]) ++Character2;
                else Character1 -= Character2, Character2 = 0;
            if (Character2 == StringTarget.CharacterSize) return Character1 - StringTarget.CharacterSize;
            return -1;
        }

        String doReplace(const String &StringTarget, const String &StringSource) const noexcept;

        String doReverse() const noexcept;

        String doStrip(char16_t CharacterSource) const noexcept;

        String doStrip(const String &StringTarget) noexcept;

        String doTruncate(intmax_t CharacterStart, intmax_t CharacterStop) const;

        char16_t getCharacter(intmax_t CharacterIndex) const;

        intmax_t getCharacterSize() const noexcept {
            return CharacterSize;
        }

        intmax_t hashCode() const noexcept override {
            uintmax_t HashCode = 0;
            for (intmax_t ElementIndex = 0; ElementIndex < CharacterSize; ++ElementIndex)
                HashCode = (HashCode << 5) - HashCode + CharacterContainer[ElementIndex];
            return (intmax_t) HashCode;
        }

        bool isContains(char16_t CharacterTarget) const noexcept {
            return doFind(CharacterTarget) != -1;
        }

        bool isContains(const String &StringTarget) const noexcept {
            return doFind(StringTarget) != -1;
        }

        bool isEmpty() const noexcept {
            return CharacterSize == 0;
        }

        bool isEndswith(const String &StringSuffix) const noexcept {
            if (StringSuffix.CharacterSize > CharacterSize) return false;
            for (intmax_t CharacterIndex = 0; CharacterIndex < StringSuffix.CharacterSize; ++CharacterIndex)
                if (CharacterContainer[CharacterIndex + CharacterSize - StringSuffix.CharacterSize] !=
                    StringSuffix.CharacterContainer[CharacterIndex])
                    return false;
            return true;
        }

        bool isNull() const noexcept {
            return CharacterContainer == nullptr;
        }

        bool isStartswith(const String &StringPrefix) const noexcept {
            if (StringPrefix.CharacterSize > CharacterSize) return false;
            for (intmax_t CharacterIndex = 0; CharacterIndex < StringPrefix.CharacterSize; ++CharacterIndex)
                if (CharacterContainer[CharacterIndex] != StringPrefix.CharacterContainer[CharacterIndex]) return false;
            return true;
        }

        String &operator=(const String &StringSource) noexcept {
            doAssign(StringSource);
            return *this;
        }

        String toLowerCase() const noexcept;

        String toUpperCase() const noexcept;

        String toString() const noexcept override {
            return *this;
        }

        std::string toU8String() const noexcept {
            static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> StringConverter;
            return StringConverter.to_bytes(CharacterContainer, CharacterContainer + CharacterSize);
        }

        std::u16string toU16String() const noexcept {
            return CharacterContainer;
        }

        std::u32string toU32String() const noexcept {
            static std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t> StringConverter;
            return StringConverter.from_bytes((const char*) CharacterContainer, (const char*) (CharacterContainer + CharacterSize));
        }

        std::wstring toWString() const noexcept {
            return {CharacterContainer, CharacterContainer + CharacterSize + 1};
        }

        template<std::derived_from<Object> T>
        static String valueOf(const T &ObjectSource) noexcept {
            return ObjectSource.toString();
        }
    };

    class StringStream final : public Object {
    private:
        uintmax_t CharacterCapacity;
        uintmax_t CharacterSize;
        char16_t *CharacterContainer;
    public:
        constexpr StringStream() : CharacterCapacity(0), CharacterSize(0), CharacterContainer(nullptr) {}

        ~StringStream() noexcept {
            doClear();
        }

        void addCharacter(char16_t CharacterSource) noexcept {
            if (CharacterCapacity == 0) CharacterContainer = new char16_t[CharacterCapacity = 1];
            if (CharacterSize == CharacterCapacity) {
                auto *CharacterBuffer = new char16_t[CharacterCapacity];
                memcpy(CharacterBuffer, CharacterContainer, sizeof(char16_t) * CharacterSize);
                delete[] CharacterContainer;
                CharacterContainer = new char16_t[CharacterCapacity <<= 1];
                memcpy(CharacterContainer, CharacterBuffer, sizeof(char16_t) * CharacterSize);
                delete[] CharacterBuffer;
            }
            CharacterContainer[CharacterSize++] = CharacterSource;
        }

        void addString(const std::u16string &StringSource) noexcept {
            if (CharacterSize + StringSource.size() > CharacterCapacity) {
                while (CharacterSize + StringSource.size() > CharacterCapacity) CharacterCapacity <<= 1;
                auto *ElementBuffer = new char16_t[CharacterSize];
                memcpy(ElementBuffer, CharacterContainer, sizeof(char16_t) * CharacterSize);
                delete[] CharacterContainer;
                CharacterContainer = new char16_t[CharacterCapacity];
                memcpy(CharacterContainer, ElementBuffer, sizeof(char16_t) * CharacterSize);
                delete[] ElementBuffer;
            }
            memcpy(CharacterContainer + CharacterSize, StringSource.data(), sizeof(char16_t) * StringSource.size());
            CharacterSize += StringSource.size();
        }

        void doClear() noexcept {
            if (CharacterCapacity && CharacterSize && CharacterContainer) {
                CharacterCapacity = 0;
                CharacterSize = 0;
                delete[] CharacterContainer;
                CharacterContainer = nullptr;
            }
        }

        String toString() const noexcept override {
            if (!CharacterContainer) return std::u16string(u"");
            return std::u16string(CharacterContainer, CharacterContainer + CharacterSize);
        }
    };
}
