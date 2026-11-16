//              Copyright Bprs Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <concepts>
#include <bit>
#include <cstdint>
#include <type_traits>

namespace bprs
{
template<typename T>
concept BaseMbInt = std::same_as<T, uint16_t>
    || std::same_as<T, int16_t>
    || std::same_as<T, uint32_t>
    || std::same_as<T, int32_t>
    || std::same_as<T, uint64_t>
    || std::same_as<T, int64_t>;

template<typename T>
concept MbInt = BaseMbInt<std::remove_cv_t<T>>;

/*!
@class Unaligned
@abstract Wrapper for a unaligned access to single value or array of basic type T.
*/
#pragma pack(push, 1)
template<MbInt T>
class Unaligned
{
public:
    using type = T;
    operator T() const noexcept { return m_value; }
    Unaligned& operator =(T val) noexcept
    {
        m_value = val;
        return *this;
    }

private:
    T m_value;
};
#pragma pack(pop)

//Swap helpers
template<typename T> struct IsUnalignedInt : std::false_type {};
template<MbInt T> struct IsUnalignedInt<Unaligned<T>> : std::true_type {};

template<typename T> concept UnalignedMbInt = MbInt<T> || IsUnalignedInt<std::remove_cv_t<T>>::value;

template<typename T> struct UnderlyingValue { using type = T; };
template<typename T> requires requires { typename T::type; }
struct UnderlyingValue<T> { using type = typename T::type; };
template<typename T>
using UnderlyingValueType = typename UnderlyingValue<T>::type;

/*!
@class Swap
@abstract Wrapper for byte-order enforcement of underlying type T, where emdian is the byte-order that is requires bytes
swap.
The default behavior is a swap of underlying type.
*/
template<UnalignedMbInt T, std::endian endian = std::endian::native>
class Swap
{
public:
    using ValueType = UnderlyingValueType<T>;

    operator ValueType() const noexcept
    {
        ValueType raw = static_cast<ValueType>(m_value);
        if constexpr (std::endian::native != endian)
            return std::byteswap(raw);
        else
            return m_value;
    }

    Swap& operator =(ValueType val) noexcept
    {
        ValueType raw = static_cast<ValueType>(val);
        if constexpr (std::endian::native != endian)
            m_value = std::byteswap(raw);
        else
            m_value = raw;
        return *this;
    }

private:
    T m_value;
};

/*!
@class Be
@abstract Wrapper for Big-endian byte-order enforcement of underlying type T.
*/
template<typename T>
using Be = Swap<T, std::endian::big>;
/*!
@class UnalignedBe
@abstract Wrapper for Big-endian byte-order enforcement of underlying type T, that can be unaligned.
*/
template<typename T>
using UnalignedBe = Be<Unaligned<T>>;

/*!
@class Le
@abstract Wrapper for Little-endian byte-order enforcement of underlying type T.
*/
template<typename T>
using Le = Swap<T, std::endian::little>;
/*!
@class Le
@abstract Wrapper for Little-endian byte-order enforcement of underlying type T, that can be unaligned..
*/
template<typename T>
using UnalignedLe = Le<Unaligned<T>>;
}
