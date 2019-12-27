#include <gtest/gtest.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "node.h"


class NodesTest : public ::testing::Test {
protected:
    void SetUp() override 
    {
        addr1_.SetIpv4(inet_addr("1.1.1.1"));
        auto node1 = std::make_shared<btclite::network::Node>(nullptr, addr1_, false);
        id1_ = node1->id();
        nodes_.AddNode(node1);
        
        addr2_.SetIpv4(inet_addr("1.1.1.2"));
        auto node2 = std::make_shared<btclite::network::Node>(nullptr, addr2_);
        id2_ = node2->id();
        nodes_.AddNode(node2);
        
        addr3_.SetIpv4(inet_addr("1.1.1.3"));
        auto node3 = nodes_.InitializeNode(nullptr, addr3_);
        id3_ = node3->id();
    }
    
    void TearDown() override
    {
        SingletonBlockSync::GetInstance().Clear();
        SingletonNodes::GetInstance().Clear();
    }
    
    btclite::network::Nodes nodes_;
    btclite::network::NetAddr addr1_;
    btclite::network::NetAddr addr2_;
    btclite::network::NetAddr addr3_;
    btclite::network::NodeId id1_ = 0;
    btclite::network::NodeId id2_ = 0;
    btclite::network::NodeId id3_ = 0;
};

class NodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        addr_.SetIpv4(inet_addr("1.1.1.1"));
        auto node = std::make_shared<btclite::network::Node>(nullptr, addr_);
        id_ = node->id();
        SingletonNodes::GetInstance().AddNode(node);
    }
    
    void TearDown() override
    {
        SingletonNodes::GetInstance().Clear();
    }
    
    btclite::network::NetAddr addr_;
    btclite::network::NodeId id_ = 0;
};