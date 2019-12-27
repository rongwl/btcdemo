#include <gtest/gtest.h>

#include "protocol/ping.h"


using namespace btclite::network::protocol;

class PingTest : public ::testing::Test {
protected:
    PingTest()
        : nonce_(0x1122334455667788), 
          ping1_(), ping2_(nonce_), ping3_(0, 0) {}
    
    uint64_t nonce_ = 0;
    Ping ping1_;
    Ping ping2_;
    Ping ping3_;
};