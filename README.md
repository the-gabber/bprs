[![Linux build status](https://github.com/the-gabber/bprs/actions/workflows/linux-builds.yml/badge.svg)](https://github.com/the-gabber/bprs/actions/workflows/linux-builds.yml)
[![MacOS build status](https://github.com/the-gabber/bprs/actions/workflows/mac-builds.yml/badge.svg)](https://github.com/the-gabber/bprs/actions/workflows/mac-builds.yml)
[![Windows build status](https://github.com/the-gabber/bprs/actions/workflows/windows-builds.yml/badge.svg)](https://github.com/the-gabber/bprs/actions/workflows/windows-builds.yml)


## What is BPRS (Binary PaRSer)

BPRS is a C++ 23+ header-only library for fast parsing of binary data, for example network or USB protocols, or any
other kind of data in binary format.
This library is based on my experience of implementing backend for real-time video stream routing for WebRTC, and
implementation of my project MTP for Mac, that is a macOS driver that works over USB protocol (noexcept version TBD).
The aim of a BPRS is a zero overhead paring/construction of binary data, with zero-overhead, but with boundary checks to
prevent buffer-overrun kink of bugs.


## Binary protocol modeling
A vast majority of protocols consist of stable blocks, each of which consists of a few variables of the same or
different types. Some of these blocks are mandatory, and some of them can be optional (depends on protocol metadata
structure).
On binary packet receive, the DataRange takes the range of data and provides typed access to its elements. There's no
reason to parse/construct packet variable by variable way, because it will require boundary check each time. The best
way is to model protocol as a structure of maximal stable size. This way the boundary check happens just once.
```text
    ----------------
    |              |
    |     A        |
    |              |
    ----------------
    |              |
    |    B (opt)   |
    |              |
    ----------------
```
On the picture above, parsing goes by 2 steps: first we request mandatory block A of the protocol (if there's not
enough space, then packet is corrupted and exception will be thrown), second step is request optional B block, but as
far it is optional it may not be present in a packet (but additional check on exact packet end may be added here).

Upon modelling protocol by a structure, not only types of fields should be taken in account, but padding and fields
alignment as well. Unaligned access on some CPU architectures may cause hardware-exception that cause program abnormal
termination. If protocol To make your code to work with such kind of data the "packing" is used.
Not all protocols require packing, if element of protocol always aligned don't use packing as far it may result that
compiler emits slower instructions for unaligned access.
But if protocol requires packing (usually the reason for that is to compress data to reduce a bottleneck in data
transfer), then a few things should be taken in account.
- If some variables in structure are not naturally-aligned, packing should be used, to prevent compiler to insert
"padding" in between variables to align.
- Even if all variables in structure are naturally-aligned, this protocol's block can follow unaligned one. If structure
doesn't fill its full occupied length, then padding at the end of structure will be inserted, and it may shift parsing
phase of next protocol's block.
- The important consequence is that you can't pass pointer or reference on variables from packed structure to other
places (functions or store and use out of its context), as far as it can be unaligned, and that is unexpected for other
code.
- If protocol uses array of elements that could be unaligned, you can't just operate as an array of basic type
(compiler expects them aligned). bprs::Unaligned wrapper should be used instead of basic type.


## Structure packing

Correct modeling of protocol requires use of correct types (for example don't use type "long" if you want 64bit value,
use exactly sized type like uint64_t or equal), and use packing to control padding. Each element in structure by default
is naturally aligned (uint8_t - is byte aligned, uint32_t - is 4 bytes aligned etc.). If address after does not satisfy
alignment of next field, then padding will be inserted to satisfy alignment requirements of next field.
Structure itself has alignment of its maximal alignment of its fields.

Examples of padding:
- Example 1: Padding in between fields of structure. Structure is 4 bytes aligned.
```text
    ----------------
    |uint8_t a     |
    |PADDING 3bytes|
    |uint32_t b    |
    ----------------
```

- Example 2: Padding at the end of the structure. Structure is 8 bytes aligned. Even though fields of structure are
naturally-aligned, because structure's alignment is bigger, padding at the end of structure will be inserted. And it may
be not what protocol expects.
```text
    ----------------
    |uint64_t a    |
    |uint32_t b    |
    |PADDING 4bytes|
    ----------------
```

As far as packing still is not in the standard nor C, nor C++. It has a few ways to be done.
- GCC and Clang variant: The most elegant one.
```cpp
    struct [[gnu::packed]] A
    {

    };
```

- GCC and Clang variant 2:
```cpp
    struct __attribute__((packed)) A
    {

    };
```

- Portable variant:
```cpp
    #pragma pack(push, 1)
    struct A
    {

    };
    #pragma pack(pop)
```

# Wrappers

BPRS provides useful wrappers for basic integer types to model byte-order and unaligned access.
- Use bprs::Unaligned to access single or an array integer values that may be unaligned.
- Use bprs::Be or bprs::Le to specify exact byte order of underlying data, Big-endian or Little-endian respectively.
- Use bprs::UnalignedBe or bprs::UnalignedLe if not only byte order should be accounted but the data alignment as well.


## How to use

As an example let's take over-simplified version of [RTP protocol](https://en.wikipedia.org/wiki/Real-time_Transport_Protocol).
Here we ignore many details.

- First let's model protocol header. As far as it's naturally-aligned, and starts at the beginning of the buffer, it
doesn't require packing.
```cpp
namespace rtp
{

class /*[[gnu::packed]]*/ Header
{
public:
    /*...*/
    size_t ccCount() const noexcept { return m_flags >> 4; }
    uint32_t ssrc() const noexcept { return m_ssrc; }
    /*...*/

private:
    uint8_t m_flags;
    uint8_t m_payloadType;
    bprs::Be<uint16_t> m_sequenceNumber; //Big-endian, aligned
    bprs::Be<uint32_t> m_timeStamp; //Big-endian, aligned
    bprs::Be<uint32_t> m_ssrc; //Big-endian, aligned
};

}
```

- After we receive buffer we pass it to bprs::DataRange for parsing.
```cpp
#include <bprs>

/*...*/
std::byte buf[MTU_SIZE];
int ret = recv(socket, buf, sizeof(bufsize), 0);
const bprs::DataRange dr(buf, ret);
```
Since now, all access to received data should go only through data-range object.

- Read the first block of data the biggest possible, to check border once per this amount of data. Then read the
dependent array.
```cpp
const auto& hdr = dr.read<rtp::Header>(); //Now we have a header
const auto csrcs = dr.readArray<uint32_t>(hdr.ccCount()); //Read full array, it is known from header
for (auto csrc: csrcs) { /*...*/ } //Do some processing
// Process headers
```

- Parse further by dividing original range on subranges. It will prevent from errors where different parts of protocol
violates boundaries of each other.
```cpp
const auto payload = dr.readRange(dr.bytesLeft());
parsePayload(payload);
```
or
```cpp
const auto payload = dr.subrange(dr.tell(), dr.bytesLeft());
parsePayload(payload);
```
It's always a good idea to use subranging for isolation of different parts of protocol. And pass ranges to dedicated
processor-functions.

The composition of packet happens the similar way.


## Liveness

Library provides class that represent a "view" concept. That means that it does not make liveness of buffer longer, and
completely depends on its lifetime. So life-time of DataRange object should be enclosed to life-time of buffer. To
provide a basic check of liveness overrun, library utilize features of Clang and MSVC (GCC doesn't support it).
But if on some reasons you want to turn it off, you can define BPRS_NO_LIFETIME.


## Exceptions

The choice of exceptions for error delivery is not only C++ natural choice, but dictated by performance reasons.
Exception mechanism sometimes misunderstood as slow. At least for Itanium-based zero-overhead implementation it's
vise-verse. For hot-path (that is without errors), it boosts performance from 3% to 10+% depending on project, by
reducing amount of error code checks spread across a code. But if exception is thrown, it may be really slow.
That means right balance. Data that is required by protocol is checked with exceptions, but parts that may not exist
should be checked with std::optional. But in same time this property of exceptions may be used for DoS attacks. That
means that early sanity checks should be provided for traffic. Usually on early stage it's possible to check for 90+% of
errors.


## Library design concept

Library's main goal is to provide mechanism to achieve maximum performance in parsing with maximum safety. It provides
tools for typed elements extraction/addition in sequential way. That enforcement is better for CPU caches as far as even
on CPU with simple prefetcher, linear memory access will work on maximum speed.
In most cases parsing and packet construction is spatial separated. And it looks that it would be right to split parsing
and construction in different classes. But in network development we can find cases, where during parsing code wants to
modify data. The example is Chromium, that during parsing on incoming RTP packet, removes extensions between header and
payload, by moving payload data next to header. To support such cases (that I strongly believe is not a best practice),
DataRange class provides functionality either for parsing and construction.


## Roadmap
- std::start_lifetime_as is currently implemented only in GCC 16, but critically unstable. Let's wait for correct
implementation in compilers.
- Noexcept version of BRPS. Will test it on my driver first.
- Add functions to parse Unicode strings.


Best regards,
Evgeny Erokhin.