#include "protocol/reject_tests.h"
#include "stream.h"


namespace btclite {
namespace unit_test {

using namespace network::protocol;

TEST_F(RejectTest, Constructor)
{
    EXPECT_EQ(reject1_.message(), "");
    EXPECT_EQ(reject1_.ccode(), CCode::kRejectUnknown);
    EXPECT_EQ(reject1_.reason(), "");
    EXPECT_EQ(reject1_.data(), crypto::null_hash);
    
    EXPECT_EQ(reject2_.message(), message_);
    EXPECT_EQ(reject2_.ccode(), ccode_);
    EXPECT_EQ(reject2_.reason(), reason_);
    EXPECT_EQ(reject2_.data(), data_);
    
    EXPECT_EQ(reject3_.message(), message_);
    EXPECT_EQ(reject3_.ccode(), ccode_);
    EXPECT_EQ(reject3_.reason(), reason_);
    EXPECT_EQ(reject3_.data(), data_);
}

TEST_F(RejectTest, OperatorEqual)
{
    EXPECT_EQ(reject2_, reject3_);
    EXPECT_NE(reject1_, reject2_);
    
    reject2_.set_message("foo");
    EXPECT_NE(reject2_, reject3_);
    
    reject2_.set_message(std::move(reject3_.message()));
    EXPECT_EQ(reject2_, reject3_);
    reject2_.set_ccode(CCode::kRejectMalformed);
    EXPECT_NE(reject2_, reject3_);
    
    reject2_.set_ccode(reject3_.ccode());
    EXPECT_EQ(reject2_, reject3_);
    reject2_.set_reason("bar");
    EXPECT_NE(reject2_, reject3_);
    
    reject2_.set_reason(std::move(reject3_.reason()));
    EXPECT_EQ(reject2_, reject3_);
    reject2_.set_data(util::RandHash256());
    EXPECT_NE(reject2_, reject3_);
    
    reject2_.set_message(msg_command::kMsgVersion);
    reject3_.set_message(msg_command::kMsgVersion);
    EXPECT_EQ(reject2_, reject3_);
}

TEST_F(RejectTest, Set)
{
    reject1_.set_message(reject2_.message());
    reject1_.set_ccode(reject2_.ccode());
    reject1_.set_reason(reject2_.reason());
    reject1_.set_data(reject2_.data());
    EXPECT_EQ(reject1_, reject2_);
    
    reject1_.set_message("");
    reject1_.set_message(std::move(reject2_.message()));
    reject1_.set_reason("");
    reject1_.set_reason(std::move(reject2_.reason()));
    EXPECT_EQ(reject1_, reject2_);
}

TEST_F(RejectTest, Clear)
{
    reject2_.Clear();
    EXPECT_EQ(reject1_, reject2_);
}

TEST_F(RejectTest, IsValid)
{
    EXPECT_FALSE(reject1_.IsValid());
    reject1_.set_ccode(CCode::kRejectInvalid);
    EXPECT_TRUE(reject1_.IsValid());
}

TEST_F(RejectTest, Serialize)
{
    std::vector<uint8_t> vec;
    util::ByteSink<std::vector<uint8_t> > byte_sink(vec);
    util::ByteSource<std::vector<uint8_t> > byte_source(vec);
    
    reject2_.Serialize(byte_sink);
    reject1_.Deserialize(byte_source);
    EXPECT_EQ(reject1_, reject2_);
    
    reject1_.Clear();
    reject2_.set_message(msg_command::kMsgVersion);
    reject2_.Serialize(byte_sink);
    reject1_.Deserialize(byte_source);
    EXPECT_EQ(reject1_, reject2_);
}

TEST_F(RejectTest, SerializedSize)
{
    util::MemoryStream ms;

    ms << reject2_;
    EXPECT_EQ(reject2_.SerializedSize(), ms.Size());
    
    ms.Clear();
    reject2_.set_message(msg_command::kMsgVersion);
    ms << reject2_;
    EXPECT_EQ(reject2_.SerializedSize(), ms.Size());
}

} // namespace unit_test
} // namespace btclit
