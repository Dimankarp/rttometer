#pragma once
#include <concepts>
#include <cstdint>
#include <iterator>
namespace rtt::serial {

//
/**

    Write @ref T value to @ref start iterator
   in @b network order bytes.

   @note @ref start is iterated by sizeof(T)

    @tparam T std::integral
    @tparam C convertible from char
    @tparam OutIt trrt::ConvertibleTypeOutputIterator<char, C>

    Taken from github.com/Dimankarp/torrtik
 */
template <std::integral T, std::output_iterator<char> OutIt>
inline void write_netord_impl(T value, OutIt& start) {
    static const int BITS_IN_BYTE = 8;
    static const unsigned char BYTE_MASK = 0xff;
    int shift = (sizeof(value) - 1) * BITS_IN_BYTE;
    for(int i = 0; i < static_cast<int>(sizeof(T)); ++i) {
        char b = (value >> shift) & BYTE_MASK;
        *start = b;
        ++start;
        shift -= BITS_IN_BYTE;
    }
}

template <typename OutIt> void write_uint64(uint64_t val, OutIt& start) {
    write_netord_impl<uint64_t>(val, start);
}

/**

    Reads @ref T value in @b native-endiannes from @ref start
   @b network order bytes iterator.

   @note @ref start is iterated by sizeof(T)

    @tparam T std::integral the type to parse to
    @tparam InIt std::input_iterator

    Taken from github.com/Dimankarp/torrtik
 */

template <std::integral T, std::input_iterator InIt>
inline T read_netord_impl(InIt& start) {

    if constexpr(sizeof(T) != 1) {
        static const int BITS_IN_BYTE = 8;
        T value = 0;
        for(int i = 0; i < static_cast<int>(sizeof(T)); ++i) {
            value <<= BITS_IN_BYTE;
            value |= static_cast<unsigned char>(*start);
            ++start;
        }
        return value;
    } else {
        // aoviding UB on opeartion(b << 8) for bytes
        return static_cast<T>(*start++);
    }
}

template <typename InIt> uint64_t read_uint64(InIt& start) {
    return read_netord_impl<uint64_t>(start);
}


} // namespace rtt::serial