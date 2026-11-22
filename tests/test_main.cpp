//              Copyright Bprs Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#include <gtest/gtest.h>
#include <bprs.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <array>

struct SomeData
{
	uint32_t data;
};

// ==================== Range Tests ====================

TEST(Range, Constructor)
{
	std::byte buf[100];
	bprs::Range r(buf, 50);

	ASSERT_EQ(r.size(), 50);
}

TEST(Range, ConstructorZeroSize)
{
	std::byte buf[100];
	bprs::Range r(buf, 0);

	ASSERT_EQ(r.size(), 0);
}

TEST(Range, InRange)
{
	std::byte buf[100];
	bprs::Range r(buf, 50);

	ASSERT_TRUE(r.inRange(0));
	ASSERT_TRUE(r.inRange(49));
	ASSERT_FALSE(r.inRange(50));
	ASSERT_FALSE(r.inRange(51));
}

TEST(Range, InRangeAtBoundary)
{
	std::byte buf[100];
	bprs::Range r(buf, 100);

	ASSERT_TRUE(r.inRange(0));
	ASSERT_TRUE(r.inRange(99));
	ASSERT_FALSE(r.inRange(100));
}

TEST(Range, IsBounded)
{
	std::byte buf[100];
	bprs::Range r(buf, 50);

	ASSERT_TRUE(r.isBounded(0));
	ASSERT_TRUE(r.isBounded(50));
	ASSERT_FALSE(r.isBounded(51));
}

TEST(Range, IsBoundedAtBoundary)
{
	std::byte buf[100];
	bprs::Range r(buf, 100);

	ASSERT_TRUE(r.isBounded(0));
	ASSERT_TRUE(r.isBounded(100));
	ASSERT_FALSE(r.isBounded(101));
}

TEST(Range, Equality)
{
	std::byte buf1[100];
	std::byte buf2[100];
	std::byte buf3[50];

	bprs::Range r1(buf1, 50);
	bprs::Range r2(buf1, 50);
	bprs::Range r3(buf2, 50);

	ASSERT_TRUE(r1 == r2);
	ASSERT_FALSE(r1 == r3);
}

TEST(Range, Inequality)
{
	std::byte buf1[100];
	std::byte buf2[100];

	bprs::Range r1(buf1, 50);
	bprs::Range r2(buf2, 50);

	ASSERT_FALSE(r1 == r2);
	ASSERT_TRUE(r1 != r2);
}

// ==================== DataRange Tests ====================

TEST(DataRange, Constructor)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 50);

	ASSERT_EQ(dr.size(), 50);
	ASSERT_EQ(dr.bytesLeft(), 50);
	ASSERT_EQ(dr.tell(), 0);
}

TEST(DataRange, ConstructorZeroSize)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 0);

	ASSERT_EQ(dr.size(), 0);
	EXPECT_EQ(dr.bytesLeft(), 0);
	EXPECT_FALSE(dr.inRange(0));
	EXPECT_TRUE(dr.isBounded(0));
	EXPECT_THROW({auto& data = dr.write<SomeData>();}, std::length_error);
	EXPECT_THROW({const auto& data = dr.read<SomeData>();}, std::length_error);
}

TEST(DataRange, SeekValid)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	dr.seek(50);
	ASSERT_EQ(dr.tell(), 50);
	ASSERT_EQ(dr.bytesLeft(), 50);
}

TEST(DataRange, SeekInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	EXPECT_THROW({dr.seek(101);}, std::length_error);
}

TEST(DataRange, SeekToBoundary)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	dr.seek(100);
	ASSERT_EQ(dr.tell(), 100);
	ASSERT_EQ(dr.bytesLeft(), 0);
}

TEST(DataRange, ReadValid)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	uint32_t expected = 0x12345678;
	dr.write<SomeData>().data = expected;

	dr.seek(0);
	const auto& res = dr.read<SomeData>();
	ASSERT_EQ(res.data, expected);
}

TEST(DataRange, ReadValidWithOffset)
{
	std::byte buf[100]{};
	bprs::DataRange dr(buf, 100);

	// Write at offset 10
	dr.seek(10);
	uint32_t expected = 0x12345678;
	dr.write<SomeData>().data = expected;

	dr.seek(10);
	const auto& res = dr.read<SomeData>();
	ASSERT_EQ(res.data, expected);
}

TEST(DataRange, ReadInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 0);

	EXPECT_THROW({const auto& res = dr.read<SomeData>();}, std::length_error);
}

TEST(DataRange, ReadInvalidThrows2)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 16);

	dr.seek(16);
	EXPECT_THROW({const auto& res = dr.read<unsigned>();}, std::length_error);
}

TEST(DataRange, ReadOptValid)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	uint32_t expected = 0x12345678;
	dr.write<SomeData>().data = expected;

	dr.seek(0);
	auto res = dr.readOpt<SomeData>();
	ASSERT_TRUE(res.has_value());
	ASSERT_EQ(res.value().get().data, expected);
}

TEST(DataRange, ReadOptInvalidReturnsNullopt)
{
	std::byte buf[100]{};
	bprs::DataRange dr(buf, 0);

	auto res = dr.readOpt<SomeData>();
	ASSERT_FALSE(res.has_value());
}

TEST(DataRange, WriteValid)
{
	std::byte buf[100]{};
	bprs::DataRange dr(buf, 100);

	uint32_t expected = 0x12345678;
	dr.write<SomeData>().data = expected;

	dr.seek(0);
	const auto& res = dr.read<SomeData>();
	ASSERT_EQ(res.data, expected);
}

TEST(DataRange, WriteInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 0);

	EXPECT_THROW({auto& wr = dr.write<SomeData>();}, std::length_error);
}

TEST(DataRange, ReadStringValid)
{
	std::byte buf[100]{};
	std::string expected = "Hello";
	std::strcpy(reinterpret_cast<char*>(buf + 10), expected.c_str());

	bprs::DataRange dr(buf, 100);

	dr.seek(10);
	auto res = dr.readString(5, false);
	ASSERT_EQ(res, expected);
	ASSERT_EQ(dr.tell(), 15);
}

TEST(DataRange, ReadStringWithZero)
{
	std::byte buf[100];
	std::string expected = "Hello";
	std::strcpy(reinterpret_cast<char*>(buf + 10), expected.c_str());

	bprs::DataRange dr(buf, 100);

	dr.seek(10);
	auto res = dr.readString(5, true);
	ASSERT_EQ(res, expected);
	ASSERT_EQ(dr.tell(), 16);
}

TEST(DataRange, ReadStringInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 0);

	EXPECT_THROW({auto str = dr.readString(10);}, std::length_error);
}

TEST(DataRange, ReadArrayValid)
{
	std::byte buf[100]{};
	bprs::DataRange dr(buf, 100);

	std::array<uint32_t, 3> expected = {{1, 2, 3}};
	auto wr = dr.writeArray<uint32_t>(3);
	std::copy(wr.begin(), wr.end(), expected.begin());

	dr.seek(0);
	auto res = dr.readArray<uint32_t>(3);
	ASSERT_EQ(res.size(), 3);
	for (size_t i = 0; i < 3; ++i)
		ASSERT_EQ(res[i], expected[i]);
}

TEST(DataRange, ReadArrayInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 9);

	EXPECT_THROW({const auto arr = dr.readArray<uint32_t>(10);}, std::length_error);
	dr.seek(9);
	EXPECT_THROW({const auto arr = dr.readArray<std::byte>(10);}, std::length_error);
}

TEST(DataRange, WriteArrayValid)
{
	std::byte buf[100]{};
	bprs::DataRange dr(buf, 100);

	std::array<uint32_t, 3> expected = {{1, 2, 3}};
	auto wr = dr.writeArray<uint32_t>(3);
	std::copy(wr.begin(), wr.end(), expected.begin());

	std::array<uint32_t, 3> actual;
	std::memcpy(actual.data(), buf, 3 * sizeof(uint32_t));

	for (size_t i = 0; i < 3; ++i)
		ASSERT_EQ(actual[i], expected[i]);
}

TEST(DataRange, WriteArrayInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 9);

	EXPECT_THROW({auto arr = dr.writeArray<uint32_t>(10);}, std::length_error);
	dr.seek(9);
	EXPECT_THROW({auto arr = dr.writeArray<std::byte>(10);}, std::length_error);
}

TEST(DataRange, SkipValid)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	dr.skip(10);
	ASSERT_EQ(dr.tell(), 10u);
	ASSERT_EQ(dr.bytesLeft(), 90u);
}

TEST(DataRange, SkipInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 0);

	EXPECT_THROW({dr.skip(1);}, std::length_error);
}

TEST(DataRange, ReadRangeValid)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	uint32_t expected = 0x12345678;
	dr.write<SomeData>().data = expected;

	dr.seek(0);
	auto sub = dr.readRange(sizeof(uint32_t));
	ASSERT_EQ(sub.size(), sizeof(uint32_t));
	ASSERT_EQ(sub.read<uint32_t>(), expected);
}

TEST(DataRange, ReadRangeInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 0);

	EXPECT_THROW({const auto rng = dr.readRange(1);}, std::length_error);
}

TEST(DataRange, WriteRangeValid)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	uint32_t expected = 0x12345678;
	dr.write<SomeData>().data = expected;

	dr.seek(0);
	auto sub = dr.writeRange(sizeof(uint32_t));
	ASSERT_EQ(sub.size(), sizeof(uint32_t));
	ASSERT_EQ(sub.read<uint32_t>(), expected);
}

TEST(DataRange, WriteRangeInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 0);

	EXPECT_THROW({auto rng = dr.writeRange(1);}, std::length_error);
}

TEST(DataRange, CopyValid)
{
	const auto example = std::byte(0x42);
	std::byte srcBuf[100]{};
	std::byte dstBuf[100]{};
	std::fill(srcBuf, srcBuf + 50, example);

	bprs::DataRange src(srcBuf, 50);
	bprs::DataRange dst(dstBuf, 100);

	src.seek(10);
	dst.copy(src);

	ASSERT_EQ(dst.tell(), 50);
	ASSERT_EQ(dst.bytesLeft(), 50);
	for (size_t i = 0; i < 50; ++i)
		ASSERT_EQ(dstBuf[i], example);
}

TEST(DataRange, CopyInvalidThrows)
{
	std::byte srcBuf[100];
	std::byte dstBuf[100];
	std::fill(srcBuf, srcBuf + 10, std::byte(0x42));

	bprs::DataRange src(srcBuf, 10);
	bprs::DataRange dst(dstBuf, 9);

	src.seek(5);  //Position doesn't affect copy

	EXPECT_THROW({dst.copy(src);}, std::length_error);
}

TEST(DataRange, MoveValid)
{
	const auto example = std::byte(0x42);
	std::byte srcBuf[100]{};
	std::byte dstBuf[100]{};
	std::fill(srcBuf, srcBuf + 50, example);

	bprs::DataRange src(srcBuf, 50);
	bprs::DataRange dst(dstBuf, 100);

	src.seek(10);

	dst.move(src);

	ASSERT_EQ(dst.tell(), 50);
	ASSERT_EQ(dst.bytesLeft(), 50);
	for (size_t i = 0; i < 50; ++i)
		ASSERT_EQ(dstBuf[i], example);
}

TEST(DataRange, MoveValidOverlap)
{
	std::byte buf[100]{};
	bprs::DataRange dr(buf, 50);

	unsigned i = 0;
	for (auto& val: dr.writeArray<std::byte>(50))
		val = std::byte(i++);

	auto src = dr.subrange(6, 10);

	dr.seek(0);
	dr.move(src);

	ASSERT_EQ(dr.tell(), 10);
	ASSERT_EQ(dr.bytesLeft(), 40);
	dr.seek(0);
	i = 6;
	for (const auto val: dr.readArray<std::byte>(10))
		ASSERT_EQ(val, std::byte(i++));
}

TEST(DataRange, MoveInvalidThrows)
{
	const auto example = std::byte(0x42);
	std::byte srcBuf[100];
	std::byte dstBuf[100];
	std::fill(srcBuf, srcBuf + 10, example);

	bprs::DataRange src(srcBuf, 10);
	bprs::DataRange dst(dstBuf, 100);

	dst.seek(95);
	EXPECT_THROW({dst.move(src);}, std::length_error);
	dst.seek(91);
	EXPECT_THROW({dst.move(src);}, std::length_error);
}

TEST(DataRange, ZeroValid)
{
	const auto example = std::byte(0x42);
	std::byte buf[100];
	std::fill(buf, buf + 50, example);

	bprs::DataRange dr(buf, 100);

	dr.seek(10);
	dr.zero(10);

	ASSERT_EQ(dr.bytesLeft(), 80);
	for (size_t i = 0; i < 10; ++i)
		ASSERT_EQ(buf[i + 10], std::byte(0x00));
}

TEST(DataRange, ZeroInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 9);

	EXPECT_THROW({dr.zero(10);}, std::length_error);
}

TEST(DataRange, SubrangeConstValid)
{
	const auto example = std::byte(0x42);
	std::byte buf[100]{};
	std::fill(buf + 5, buf + 15, example);

	bprs::DataRange dr(buf, 100);

	dr.seek(10);

	auto sub = dr.subrange(5, 10);
	ASSERT_EQ(sub.size(), 10);

	dr.seek(5);
	for (auto val: sub.readArray<std::byte>(10))
		ASSERT_EQ(val, example);
}

TEST(DataRange, SubrangeConstInvalidThrows)
{
	std::byte buf[100];
	const bprs::DataRange dr(buf, 100);

	EXPECT_THROW({auto sr = dr.subrange(91, 10);}, std::length_error);
}

TEST(DataRange, SubrangeInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	EXPECT_THROW({auto sr = dr.subrange(91, 10);}, std::length_error);
}

TEST(DataRange, SubrangeMutableValid)
{
	const auto example = std::byte(0x42);
	std::byte buf[100]{};
	std::fill(buf, buf + 50, example);

	bprs::DataRange dr(buf, 100);

	dr.seek(10);
	auto sub = dr.subrange(5, 10);

	ASSERT_EQ(sub.size(), 10);
	for (auto& val: sub.writeArray<std::byte>(10))
		val = std::byte(0x55);

	const auto& res = dr.read<SomeData>();
	ASSERT_EQ(res.data, 0x55555555);
}

TEST(DataRange, SubrangeEqual)
{
	std::byte buf[100]{};

	bprs::DataRange dr(buf, 100);

	const auto sub = dr.subrange(5, 10);
	const auto sub1 = sub.subrange(0, 10);
	ASSERT_EQ(sub, sub1);

	auto sub2 = dr.subrange(5, 10);
	auto sub3 = sub2.subrange(0, 10);
	ASSERT_EQ(sub2, sub3);
	ASSERT_EQ(sub, sub3);
}

TEST(DataRange, SubrangeMutableInvalidThrows)
{
	std::byte buf[100];
	bprs::DataRange dr(buf, 100);

	dr.seek(45);

	EXPECT_THROW({auto sr = dr.subrange(99, 10);}, std::length_error);
}

TEST(DataRange, SubrangeOverflow)
{
	std::byte buf[100]{};

	bprs::DataRange dr(buf, 100);

	size_t bufVal = reinterpret_cast<size_t>(buf);
	size_t tillOverflow = std::numeric_limits<size_t>::max() - bufVal;

	EXPECT_THROW({auto sr = dr.subrange(10, tillOverflow + 1);}, std::length_error);
	EXPECT_THROW({auto sr = dr.subrange(tillOverflow + 1, 10);}, std::length_error);
}

TEST(DataRange, SubrangeConstOverflow)
{
	std::byte buf[100]{};

	const bprs::DataRange dr(buf, 100);

	size_t bufVal = reinterpret_cast<size_t>(buf);
	size_t tillOverflow = std::numeric_limits<size_t>::max() - bufVal;

	EXPECT_THROW({auto sr = dr.subrange(10, tillOverflow + 1);}, std::length_error);
	EXPECT_THROW({auto sr = dr.subrange(tillOverflow + 1, 10);}, std::length_error);
}

// ==================== Wrapper Tests ====================

TEST(Unaligned, Read)
{
	uint8_t buf[9] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
	const bprs::DataRange dr(reinterpret_cast<std::byte*>(buf), 9);

	dr.seek(1);
	auto val = dr.read<bprs::Unaligned<uint16_t>>();
	if constexpr (std::endian::native != std::endian::little)
		ASSERT_EQ(val, 0x2233);
	else
		ASSERT_EQ(val, 0x3322);

	dr.seek(1);
	auto val32 = dr.read<bprs::Unaligned<uint32_t>>();
	if constexpr (std::endian::native != std::endian::little)
		ASSERT_EQ(val32, 0x22334455);
	else
		ASSERT_EQ(val32, 0x55443322);

	dr.seek(1);
	auto val64 = dr.read<bprs::Unaligned<uint64_t>>();
	if constexpr (std::endian::native != std::endian::little)
		ASSERT_EQ(val64, 0x2233445566778899);
	else
		ASSERT_EQ(val64, 0x9988776655443322);
}

TEST(Swap, SwapRead)
{
	uint8_t buf[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	const bprs::DataRange dr(reinterpret_cast<std::byte*>(buf), 8);

	auto val = dr.read<bprs::Be<uint16_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val, 0x1122);
	else
		ASSERT_EQ(val, 0x2211);

	val = dr.read<bprs::Le<uint16_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val, 0x4433);
	else
		ASSERT_EQ(val, 0x3344);

	dr.seek(0);
	auto val32 = dr.read<bprs::Be<uint32_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val32, 0x11223344);
	else
		ASSERT_EQ(val32, 0x44332211);

	dr.seek(0);
	val32 = dr.read<bprs::Le<uint32_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val32, 0x44332211);
	else
		ASSERT_EQ(val32, 0x11223344);

	dr.seek(0);
	auto val64 = dr.read<bprs::Be<uint64_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val64, 0x1122334455667788);
	else
		ASSERT_EQ(val64, 0x8877665544332211);

	dr.seek(0);
	val64 = dr.read<bprs::Le<uint64_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val64, 0x8877665544332211);
	else
		ASSERT_EQ(val64, 0x1122334455667788);
}

TEST(Swap, SwapWrite)
{
	uint8_t buf[8]{};
	bprs::DataRange dr(reinterpret_cast<std::byte*>(buf), 8);

	dr.write<bprs::Be<uint16_t>>() = 0x1122;
	ASSERT_EQ(buf[0], 0x11);
	ASSERT_EQ(buf[1], 0x22);

	dr.seek(0);
	dr.write<bprs::Le<uint16_t>>() = 0x1122;
	ASSERT_EQ(buf[0], 0x22);
	ASSERT_EQ(buf[1], 0x11);

	dr.seek(0);
	dr.write<bprs::Be<uint32_t>>() = 0x11223344;
	ASSERT_EQ(buf[0], 0x11);
	ASSERT_EQ(buf[1], 0x22);
	ASSERT_EQ(buf[2], 0x33);
	ASSERT_EQ(buf[3], 0x44);

	dr.seek(0);
	dr.write<bprs::Le<uint32_t>>() = 0x11223344;
	ASSERT_EQ(buf[0], 0x44);
	ASSERT_EQ(buf[1], 0x33);
	ASSERT_EQ(buf[2], 0x22);
	ASSERT_EQ(buf[3], 0x11);

	dr.seek(0);
	dr.write<bprs::Be<uint64_t>>() = 0x1122334455667788;
	ASSERT_EQ(buf[0], 0x11);
	ASSERT_EQ(buf[1], 0x22);
	ASSERT_EQ(buf[2], 0x33);
	ASSERT_EQ(buf[3], 0x44);
	ASSERT_EQ(buf[4], 0x55);
	ASSERT_EQ(buf[5], 0x66);
	ASSERT_EQ(buf[6], 0x77);
	ASSERT_EQ(buf[7], 0x88);

	dr.seek(0);
	dr.write<bprs::Le<uint64_t>>() = 0x1122334455667788;
	ASSERT_EQ(buf[7], 0x11);
	ASSERT_EQ(buf[6], 0x22);
	ASSERT_EQ(buf[5], 0x33);
	ASSERT_EQ(buf[4], 0x44);
	ASSERT_EQ(buf[3], 0x55);
	ASSERT_EQ(buf[2], 0x66);
	ASSERT_EQ(buf[1], 0x77);
	ASSERT_EQ(buf[0], 0x88);
}

TEST(Swap, UnalignedRead)
{
	uint8_t buf[9] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
	const bprs::DataRange dr(reinterpret_cast<std::byte*>(buf), 9);

	dr.seek(1);
	auto val = dr.read<bprs::UnalignedBe<uint16_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val, 0x2233);
	else
		ASSERT_EQ(val, 0x3322);

	dr.seek(1);
	val = dr.read<bprs::UnalignedLe<uint16_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val, 0x3322);
	else
		ASSERT_EQ(val, 0x2233);

	dr.seek(1);
	auto val32 = dr.read<bprs::UnalignedBe<uint32_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val32, 0x22334455);
	else
		ASSERT_EQ(val32, 0x55443322);

	dr.seek(1);
	val32 = dr.read<bprs::UnalignedLe<uint32_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val32, 0x55443322);
	else
		ASSERT_EQ(val32, 0x22334455);

	dr.seek(1);
	auto val64 = dr.read<bprs::UnalignedBe<uint64_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val64, 0x2233445566778899);
	else
		ASSERT_EQ(val64, 0x9988776655443322);

	dr.seek(1);
	val64 = dr.read<bprs::UnalignedLe<uint64_t>>();
	if constexpr (std::endian::native == std::endian::little)
		ASSERT_EQ(val64, 0x9988776655443322);
	else
		ASSERT_EQ(val64, 0x2233445566778899);
}

TEST(Swap, UnalignedWrite)
{
	uint8_t buf[9]{};
	bprs::DataRange dr(reinterpret_cast<std::byte*>(buf), 9);

	dr.seek(1);
	dr.write<bprs::UnalignedBe<uint16_t>>() = uint16_t(0x1122);
	ASSERT_EQ(buf[1], 0x11);
	ASSERT_EQ(buf[2], 0x22);

	dr.seek(1);
	dr.write<bprs::UnalignedLe<uint16_t>>() = 0x1122;
	ASSERT_EQ(buf[1], 0x22);
	ASSERT_EQ(buf[2], 0x11);

	dr.seek(1);
	dr.write<bprs::UnalignedBe<uint32_t>>() = 0x11223344;
	ASSERT_EQ(buf[1], 0x11);
	ASSERT_EQ(buf[2], 0x22);
	ASSERT_EQ(buf[3], 0x33);
	ASSERT_EQ(buf[4], 0x44);

	dr.seek(1);
	dr.write<bprs::UnalignedLe<uint32_t>>() = 0x11223344;
	ASSERT_EQ(buf[1], 0x44);
	ASSERT_EQ(buf[2], 0x33);
	ASSERT_EQ(buf[3], 0x22);
	ASSERT_EQ(buf[4], 0x11);

	dr.seek(1);
	dr.write<bprs::UnalignedBe<uint64_t>>() = 0x1122334455667788;
	ASSERT_EQ(buf[1], 0x11);
	ASSERT_EQ(buf[2], 0x22);
	ASSERT_EQ(buf[3], 0x33);
	ASSERT_EQ(buf[4], 0x44);
	ASSERT_EQ(buf[5], 0x55);
	ASSERT_EQ(buf[6], 0x66);
	ASSERT_EQ(buf[7], 0x77);
	ASSERT_EQ(buf[8], 0x88);

	dr.seek(1);
	dr.write<bprs::UnalignedLe<uint64_t>>() = 0x1122334455667788;
	ASSERT_EQ(buf[8], 0x11);
	ASSERT_EQ(buf[7], 0x22);
	ASSERT_EQ(buf[6], 0x33);
	ASSERT_EQ(buf[5], 0x44);
	ASSERT_EQ(buf[4], 0x55);
	ASSERT_EQ(buf[3], 0x66);
	ASSERT_EQ(buf[2], 0x77);
	ASSERT_EQ(buf[1], 0x88);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
