#include <bprs.h>
#include <iostream>
#include <string_view>

using namespace std::literals::string_view_literals;

/*
In this example over-simplified construction and parsing of RTP protocol's header.
This code is just to show logic of BPRS on construction and parsing.
*/

namespace rtp
{

/*
First model data layout and byte-ordering (here we have naturally-aligned header, so no packing is required).
Second, is to provide as high level interface to protocol's block as possible. This way we try to prevent errors on the
lowest possible level.
*/
class /*[[gnu::packed]]*/ Header
{
public:
    void init()
    {
        m_flags = 1;
    }
    void setPt(uint8_t pt, bool marker = false) { m_payloadType = pt & (marker << 7); }
    void setSsrc(uint32_t ssrc) { m_ssrc = ssrc; }
    void setTimeStamp(uint32_t ts) { m_timeStamp = ts; }

    size_t ccCount() const noexcept { return m_flags >> 4; }
    uint32_t timeStamp() const noexcept { return m_timeStamp; }
    uint32_t ssrc() const noexcept { return m_ssrc; }
    uint8_t version() const noexcept { return m_flags & 0x3; }
    bool marker() const noexcept { return m_payloadType >> 7; }
    uint8_t type() const noexcept { return m_payloadType & 0x80; }

private:
    uint8_t m_flags;
    uint8_t m_payloadType;
    bprs::Be<uint16_t> m_sequenceNumber;
    bprs::Be<uint32_t> m_timeStamp;
    bprs::Be<uint32_t> m_ssrc;
};

}

//Payload composed separately, as one of compose steps.
void constructPayload(bprs::DataRange& dr)
{
    constexpr auto testStr = "Hello RTP!"sv;
    dr.copy(reinterpret_cast<const std::byte*>(testStr.data()), testStr.length());
}

size_t composePacket(std::byte* buf, size_t len)
{
    bprs::DataRange dr(buf, len);
    auto& hdr = dr.write<rtp::Header>();
    hdr.init();
    hdr.setSsrc(123);
    hdr.setPt(13);
    hdr.setTimeStamp(20167);

    constructPayload(dr);

    return dr.tell();
}

//Payload parsed separately, as one of parsing steps.
void payloadParser(const bprs::DataRange& dr)
{
    const auto sv = dr.readString(dr.bytesLeft(), false);
    std::cout << "Payload: " << sv << std::endl;
}

void parsePacket(std::byte* buf, size_t len)
{
    const bprs::DataRange dr(buf, len);
    const auto& hdr = dr.read<rtp::Header>();

    std::cout << "Parse RTP packer header:\n\tversion: " << (unsigned) hdr.version()
        << "\n\tssrc: " << hdr.ssrc()
        << "\n\ttype: " << (unsigned) hdr.type()
        << "\n\ttime stamp: " << hdr.timeStamp()
        << "\n\textensions: " << hdr.ccCount()
        << std::endl;

    payloadParser(dr);
}

int main(int argc, char** argv)
{
    std::byte buf[512];

    auto len = composePacket(buf, sizeof(buf));
    parsePacket(buf, len);

    return 0;
}
