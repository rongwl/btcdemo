#include "network/include/params.h"

#include <gtest/gtest.h>
#include <vector>

#include "btcnet.h"
#include "constants.h"


namespace btclite {
namespace unit_test {

using namespace network;

TEST(NetworkParamsTest, Constructor)
{
    Params network_params(BtcNet::kMainNet, util::Args(), fs::path("/tmp/foo"));
    EXPECT_EQ(kMainnetMagic, network_params.msg_magic());
    EXPECT_EQ(8333, network_params.default_port());    
    std::vector<Seed> vec = 
    { { "seed.bitcoin.sipa.be", 8333 },
        { "dnsseed.bluematt.me", 8333 },
        { "dnsseed.bitcoin.dashjr.org", 8333 },
        { "seed.bitcoinstats.com", 8333 },
        { "seed.bitcoin.jonasschnelli.ch", 8333 },
        { "seed.btc.petertodd.org", 8333 },
        { "seed.bitcoin.sprovoost.nl", 8333 },
    { "dnsseed.emzy.de", 8333 } };
    EXPECT_EQ(vec, network_params.seeds());
    
    Params network_params2(BtcNet::kTestNet, util::Args(), fs::path("/tmp/foo"));
    EXPECT_EQ(kTestnetMagic, network_params2.msg_magic());
    EXPECT_EQ(18333, network_params2.default_port());    
    std::vector<Seed> vec2 = 
    { { "testnet-seed.bitcoin.jonasschnelli.ch", 18333 },
        { "seed.tbtc.petertodd.org", 18333 },
        { "seed.testnet.bitcoin.sprovoost.nl", 18333 },
    { "testnet-seed.bluematt.me", 18333 } };
    EXPECT_EQ(vec2, network_params2.seeds());
    
    Params network_params3(BtcNet::kRegTest, util::Args(), fs::path("/tmp/foo"));
    EXPECT_EQ(kRegtestMagic, network_params3.msg_magic());
    EXPECT_EQ(18444, network_params3.default_port());
    EXPECT_EQ(0, network_params3.seeds().size());
    
    Params network_params4(BtcNet::kNone, util::Args(), fs::path("/tmp/foo"));
    EXPECT_EQ(0, network_params4.msg_magic());
    EXPECT_EQ(0, network_params4.default_port());
    EXPECT_EQ(0, network_params4.seeds().size());
}

} // namespace unit_test
} // namespace btclite
