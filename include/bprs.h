//              Copyright Bprs Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <cstddef>
#include <new>
#include <memory>
#include <span>
#include <optional>
#include <functional>
#include <string_view>
#include <stdexcept>
#include <version>
#include <cstring>
#include <limits>
#include "wrappers.h"

#ifndef BPRS_NO_LIFETIME
	#if defined(__clang__)
		#define LIFETIMEBOUND [[clang::lifetimebound]]
	#elif defined(_MSC_VER)
		#define LIFETIMEBOUND [[msvc::lifetimebound]]
	#else
		#define LIFETIMEBOUND
	#endif
#else
	#define LIFETIMEBOUND
#endif

namespace bprs
{

/*!
@class Range
@abstract Provides base for range checking and buffer accounting.
*/
class Range
{
public:
	Range(std::byte* buf, size_t size) noexcept : m_buf(buf), m_end(buf + size) {}

	/*! Return size of range.
	@result Size of the range in bytes.
	*/
	size_t size() const noexcept { return m_end - m_buf; }

	/*! Check if offset in range exclusive [,).
	@param offset Tested offset in bytes.
	@result true if value off in range.
	*/
	bool inRange(size_t offset) const noexcept { return offset < size(); }

	/*! Check if offset in range inclusive [,].
	@param offset Tested offset in bytes.
	@result true if offset in range.
	*/
	bool isBounded(size_t offset) const noexcept { return offset <= size(); }

	/*! Return equality of ranges.
	@result true if ranges are equal.
	*/
	bool operator ==(const Range& range) const = default;

protected:
	std::byte* buffer() noexcept { return m_buf; }
	const std::byte* buffer() const noexcept { return m_buf; }
	const std::byte* end() const noexcept { return m_end; }

private:
	std::byte* const m_buf;
	std::byte* const m_end;
};

/*!
@class DataRange
@abstract Data access class for fast binary parsing and data construction. Class provides typed access to
any kind of data in provided buffer. For correct work with binary protocols it's required to correctly 
model data layout, that requires correct "packing" and alignment. If covered structure is unaligned or contains
unaligned fields, it's better to access it through struct with "packed" attribute. It makes compiler to emit
proper instruction with only one range check per structure.
*/
class DataRange : public Range
{
public:
	/*! Constructor.
	@param buff Pointer to data for processing.
	@param size Size of the range.
	*/
	DataRange(std::byte* buf, size_t size) noexcept : Range(buf, size), m_position(buf) {}

	/*! Return equality of data ranges, position is ignored.
	@result true if data ranges are equal.
	*/
	bool operator==(const Range& other) const
	{
		return static_cast<const Range&>(*this) == other;
	}

	/*! Check if range from current offset with size of type T is available in the buffer. And if
	so it returns reference on data read only. Parsing position will be advanced on T size.
	@result Constant reference to data which we are reading.
	@throws std::length_error on range violation.
	*/
	template<typename T>
	[[nodiscard]] const T& read() const LIFETIMEBOUND
	{
		if (!isBoundedLen(sizeof(T)))
			throw std::length_error("Data read overflow");

		const auto& data = *pointer<const T>();
		m_position += sizeof(T);
		return data;
	}

	/*! Check if range from current offset with size of type T is available in the buffer. And if
	so it returns reference on data read only. Parsing position will be advanced on T size.
	@result Constant reference to data which we are reading.
	@throws std::length_error on range violation.
	*/
	template<typename T>
	[[nodiscard]] const std::optional<std::reference_wrapper<const T>> readOpt() const LIFETIMEBOUND
	{
		if (!isBoundedLen(sizeof(T)))
			return std::nullopt;

		const auto& data = *pointer<const T>();
		m_position += sizeof(T);
		return std::cref(data);
	}

	/*! Check if range from current offset with size of type T is available in the buffer. And if
	so it returns reference on data for write. On call write position will be advanced on T size.
	@result Reference to data which we are writing.
	@throws std::length_error on range violation.
	*/
	template<typename T>
	[[nodiscard]] T& write() LIFETIMEBOUND
	{
		if (!isBoundedLen(sizeof(T)))
			throw std::length_error("Data write access overflow");

		auto& data = *pointer<T>();
		m_position += sizeof(T);
		return data;
	}

	/*! Check if range from current offset with length of string is available in the buffer. And if
	so it returns pointer to string. It doesn't check for '\0', so any checks for '\0' if needed
	should be performed on callers side. Parsing position will be advanced on length.
	@param length Requested string length.
	@param addZero Compensate trailing \0. For Pascal-strings should be false.
	@result Reference to data for writing.
	@throws std::length_error on range violation.
	*/
	[[nodiscard]] std::string_view readString(size_t length, bool addZero = true) const LIFETIMEBOUND
	{
		size_t fullLen = length + (addZero ? 1 : 0);
		if (!isBoundedLen(fullLen))
			throw std::length_error("Data access overflow on string");

		std::string_view sw(pointer<char>(), length);
		m_position += fullLen;
		return sw;
	}

	/*! Check if range from current offset with length of array is available in the buffer. And if
	so it returns span object. Parsing position will be advanced on length.
	@param length Requested string length.
	@result Data span to read.
	@throws std::length_error on range violation.
	*/
	template<typename T>
	[[nodiscard]] const std::span<T> readArray(size_t count) const LIFETIMEBOUND
	{
		auto length = sizeof(T) * count;
		if (!isBoundedLen(length))
			throw std::length_error("Data access overflow on array");

		const std::span<T> arr(pointer<T>(count), count);
		m_position += length;
		return arr;
	}

	/*! Check if range from current offset with length of array is available in the buffer. And if
	so it returns span object. Parsing position will be advanced on length.
	@param length Requested string length.
	@result Pointer to data to write.
	@throws std::length_error on range violation.
	*/
	template<typename T>
	[[nodiscard]] std::span<T> writeArray(size_t count) LIFETIMEBOUND
	{
		auto length = sizeof(T) * count;
		if (!isBoundedLen(length))
			throw std::length_error("Data access overflow on array");

		std::span<T> arr(pointer<T>(count), count);
		m_position += length;
		return arr;
	}

	/*! Check if range from current offset with length, if range is available in the buffer.
	Parsing position will be advanced on length skipping required amount of data.
	@param length Requested range length.
	@throws std::length_error on range violation.
	*/
	void skip(size_t length) const
	{
		if (!isBoundedLen(length))
			throw std::length_error("Data range access overflow");

		m_position += length;
	}

	/*! Check if range from current offset with length if range is available in the buffer. And if
	so it returns new data range for read. Parsing position will be advanced on length.
	@param length Requested range length.
	@result New data range of size length.
	@throws std::length_error on range violation.
	*/
	[[nodiscard]] const DataRange readRange(size_t length) const LIFETIMEBOUND
	{
		if (!isBoundedLen(length))
			throw std::length_error("Data range access overflow");

		DataRange dr(pointer(), length);
		m_position += length;
		return dr;
	}

	/*! Check if range from current offset with length if range is available in the buffer. And if
	so it returns new data range for write. Parsing position will be advanced on length.
	@param length Requested range length.
	@result New data range of size length.
	@throws std::length_error on range violation.
	*/
	[[nodiscard]] DataRange writeRange(size_t length) LIFETIMEBOUND
	{
		if (!isBoundedLen(length))
			throw std::length_error("Data range access overflow");

		DataRange dr(pointer(), length);
		m_position += length;
		return dr;
	}

	/*! Check if range from current offset with requested length is available in the buffer. And if
	so it copies data from buffer to current buffer range from offset with requested length.
	Parsing position will be advanced on length.
	@param buffer Source buffer.
	@param length Requested string length.
	@result Reference to data which we are writing.
	@throws std::length_error on range violation.
	*/
	void copy(const std::byte* buffer, size_t length)
	{
		if (!isBoundedLen(length))
			throw std::length_error("Data access overflow on copy");

		std::memcpy(pointer(), buffer, length);
		m_position += length;
	}

	/*! Check if range from current offset with source range length is available in the buffer. And
	if so it copies data from source range to current buffer. Source range data should not
	overlap with current data range. Parsing position will be advanced on length.
	@param source Source range.
	@result Reference to data which we are writing.
	@throws std::length_error on range violation.
	*/
	void copy(const DataRange& source)
	{
		copy(source.buffer(), source.size());
	}

	/*! Check if range from current offset with source range length is available in the buffer.
	And if so it moves data from source rangeto current buffer. Source range data may overlap
	with current data range. Parsing position will be advanced on length.
	@param source Source range.
	@result Reference to data which we are writing.
	@throws std::length_error on range violation.
	*/
	void move(const DataRange& source)
	{
		if (!isBoundedLen(source.size()))
			throw std::length_error("Data access overflow on move");

		std::memmove(pointer(), source.buffer(), source.size());
		m_position += source.size();
	}

	/*! Check if range from current offset with requested length is available in the buffer. And if
	so it zeroing from current position length bytes. Parsing position will be advanced on length.
	@param length Requested zeroing length.
	@throws std::length_error on range violation.
	*/
	void zero(size_t length)
	{
		if (!isBoundedLen(length))
			throw std::length_error("Data access overflow on zeroing");

		std::memset(pointer(), 0, length);
		m_position += length;
	}

	/*! Change current offset for read/write.
	@param offset New offset for read/write.
	@throws std::length_error on range violation.
	*/
	void seek(size_t offset) const
	{
		if (!isBounded(offset))
			throw std::length_error("Bad seek offset");

		m_position = const_cast<std::byte*>(buffer()) + offset;
	}

	/*! Tells current position of read/write.
	@result Offset in range.
	*/
	size_t tell() const noexcept { return m_position - buffer(); }

	/*! Return how many bytes left till the end of range from current position.
	@result Bytes left.
	*/
	size_t bytesLeft() const noexcept { return end() - m_position; }

	/*! Returns constant subrange of current range. Doesn't change parsing position.
	@param offset Offset for new range from start of current range.
	@param size New range size.
	@result New constant subrange of current.
	@throws std::length_error on range violation.
	*/
	[[nodiscard]] const DataRange subrange(size_t offset, size_t size) const LIFETIMEBOUND
	{
		if (offset > std::numeric_limits<size_t>::max() - size //Check overflow
			|| !isBounded(offset + size))
			throw std::length_error("Bad subrange");

		return {const_cast<std::byte*>(buffer()) + offset, size};
	}

	/*! Returns subrange of current range. Doesn't change parsing position.
	@param offset Offset for new range from start of current range.
	@param size New range size.
	@result New subrange of current.
	@throws std::length_error on range violation.
	*/
	[[nodiscard]] DataRange subrange(size_t offset, size_t size) LIFETIMEBOUND
	{
		if (offset > std::numeric_limits<size_t>::max() - size //Check overflow
			|| !isBounded(offset + size))
			throw std::length_error("Bad subrange");

		return {buffer() + offset, size};
	}

private:
//FIXME: currently implemented in GCC 16, but implementation is unstable (TBD).
#if defined(ENABLE_START_LIFETIME) && defined(__cpp_lib_start_lifetime_as) && __cpp_lib_start_lifetime_as >= 202207L
	template <typename T = std::byte>
	T* pointer() const noexcept { return std::start_lifetime_as<T>(m_position); }

	template <typename T>
	T* pointer(size_t count) const noexcept { return std::start_lifetime_as_array<T>(m_position, count); }
#else
	template <typename T = std::byte>
	T* pointer() const noexcept { return std::launder(reinterpret_cast<T*>(m_position)); }

	template <typename T>
	T* pointer(size_t count) const noexcept { return std::launder(reinterpret_cast<T*>(m_position)); }
#endif

	bool isBoundedLen(size_t length) const noexcept { return length <= bytesLeft(); }

	mutable std::byte* m_position;
};

}
