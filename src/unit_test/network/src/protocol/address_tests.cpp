#include "protocol/address_tests.h"
#include "stream.h"


TEST_F(AddrTest, Validate)
{
    EXPECT_FALSE(msg_addr1_.IsValid());
    EXPECT_TRUE(msg_addr2_.IsValid());
}

TEST_F(AddrTest, Clear)
{
    ASSERT_NE(msg_addr1_, msg_addr2_);
    msg_addr2_.Clear();
    EXPECT_EQ(msg_addr1_, msg_addr2_);
}

TEST_F(AddrTest, Serialize)
{
    std::vector<uint8_t> vec;
    ByteSink<std::vector<uint8_t> > byte_sink(vec);
    ByteSource<std::vector<uint8_t> > byte_source(vec);
    msg_addr1_.Serialize(byte_sink);
    msg_addr2_.Deserialize(byte_source);
    EXPECT_EQ(msg_addr1_, msg_addr2_);
}

TEST_F(AddrTest, SerializedSize)
{
    MemOstream ms;

    ms << msg_addr2_;
    EXPECT_EQ(msg_addr2_.SerializedSize(), ms.vec().size());
}

