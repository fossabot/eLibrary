#pragma once

namespace eLibrary {
    template<typename T>
    concept Comparable = requires (const T &ObjectSource) {
        ObjectSource.doCompare(ObjectSource);
    };

    template<typename T>
    concept Hashable = requires (const T &ObjectSource) {
        ObjectSource.hashCode();
    };
}

namespace std {
    template<eLibrary::Comparable T>
    struct equal_to<T> {
        bool operator()(const T &Object1, const T &Object2) const noexcept {
            return !Object1.doCompare(Object2);
        }
    };

    template<eLibrary::Comparable T>
    struct less<T> {
        bool operator()(const T &Object1, const T &Object2) const noexcept {
            return Object1.doCompare(Object2) < 0;
        }
    };

    template<eLibrary::Comparable T>
    struct less_equal<T> {
        bool operator()(const T &Object1, const T &Object2) const noexcept {
            return Object1.doCompare(Object2) <= 0;
        }
    };

    template<eLibrary::Comparable T>
    struct greater<T> {
        bool operator()(const T &Object1, const T &Object2) const noexcept {
            return Object1.doCompare(Object2) > 0;
        }
    };

    template<eLibrary::Comparable T>
    struct greater_equal<T> {
        bool operator()(const T &Object1, const T &Object2) const noexcept {
            return Object1.doCompare(Object2) >= 0;
        }
    };

    template<eLibrary::Hashable T>
    struct hash<T> {
        auto operator()(const T &ObjectSource) const noexcept {
            return ObjectSource.hashCode();
        }
    };

    template<eLibrary::Comparable T>
    struct not_equal_to<T> {
        bool operator()(const T &Object1, const T &Object2) const noexcept {
            return Object1.doCompare(Object2);
        }
    };
}
