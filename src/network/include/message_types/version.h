#ifndef BTCLITE_MESSAGE_TYPES_VERSION_H
#define BTCLITE_MESSAGE_TYPES_VERSION_H


#include "address.h"
#include "protocol.h"
#include "serialize.h"
#include "stream.h"


namespace btclite {
namespace network {
namespace message_types {

constexpr uint32_t kProtocolVersion = 70015;

//! initial proto version, to be increased after version/verack negotiation
constexpr uint32_t kInitProtoVersion = 209;

//! In this version, 'getheaders' was introduced.
constexpr uint32_t kGetheadersVersion = 31800;

//! disconnect from peers older than this proto version
constexpr uint32_t kMinPeerProtoVersion = kGetheadersVersion;

//! timestamp field added to NetAddr, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
constexpr uint32_t kAddrTimeVersion = 31402;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
constexpr uint32_t kBip31Version = 60000;

//! "filter*" commands are disabled without kNodeBloom after and including this version
constexpr uint32_t kNoBloomVersion = 70011;

//! "sendheaders" command and announcing blocks with headers starts with this version
constexpr uint32_t kSendheadersVersion = 70012;

//! "feefilter" tells peers to filter invs to you by fee starts with this version
constexpr uint32_t kFeefilterVersion = 70013;

//! short-id-based block download starts with this version
constexpr uint32_t kShortIdsBlocksVersion = 70014;

//! not banning for invalid compact blocks starts with this version
constexpr uint32_t kInvalidCbNoBanVersion = 70015;


class Version {
public:    
    /*template <size_t N>
    struct RawData {
        int32_t version_;
        uint64_t services_;
        int64_t timestamp_;
        btc_message::NetAddr addr_recv_;
        btc_message::NetAddr addr_from_;
        uint64_t nonce_;
        btc_message::VarStr<N, VarIntSize> user_agent_;
        int32_t start_height_;
        bool relay_;
    };*/
    static const std::string kCommand;
    
    //-------------------------------------------------------------------------
    Version()
        : version_(0), services_(0), timestamp_(0), addr_recv_(),
          addr_from_(), nonce_(0), user_agent_(), start_height_(0),
          relay_(false) {}
    
    Version(uint32_t version, uint64_t services, uint64_t timestamp,
            const NetAddr& address_receiver,
            const NetAddr& address_from, uint64_t nonce,
            const std::string& user_agent, uint32_t start_height, bool relay)
        : version_(version), services_(services), timestamp_(timestamp),
          addr_recv_(address_receiver), addr_from_(address_from),
          nonce_(nonce), user_agent_(user_agent), start_height_(start_height),
          relay_(relay) {}
    
    Version(uint32_t version, uint64_t services, uint64_t timestamp,
            NetAddr&& address_receiver, NetAddr&& address_from,
            uint64_t nonce, std::string&& user_agent, uint32_t start_height,
            bool relay)
        : version_(version), services_(services), timestamp_(timestamp),
          addr_recv_(std::move(address_receiver)),
          addr_from_(std::move(address_from)),
          nonce_(nonce), user_agent_(std::move(user_agent)),
          start_height_(start_height), relay_(relay) {}
    
    explicit Version(const Version& version)
        : Version(version.version_, version.services_, version.timestamp_,
                  version.addr_recv_, version.addr_from_, version.nonce_,
                  version.user_agent_, version.start_height_, version.relay_) {}
    
    explicit Version(Version&& version) noexcept
        : Version(version.version_, version.services_, version.timestamp_,
                  std::move(version.addr_recv_),
                  std::move(version.addr_from_), version.nonce_,
                  std::move(version.user_agent_), version.start_height_,
                  version.relay_) {}
    
    explicit Version(const uint8_t *raw)
        : Version()
    {
        //UnSerialize(raw);
    }
    
    //-------------------------------------------------------------------------
    bool IsValid() const;
    void Clear();
    
    //-------------------------------------------------------------------------
    Version& operator=(const Version& b);
    Version& operator=(Version&& b) noexcept;
    
    //-------------------------------------------------------------------------
    template <typename Stream>
    void Serialize(Stream& out) const;
    template <typename Stream>
    void UnSerialize(Stream& in);
    
    //-------------------------------------------------------------------------
    uint32_t version() const
    {
        return version_;
    }
    void set_version(uint32_t version)
    {
        version_ = version;
    }

    uint64_t services() const
    {
        return services_;
    }
    void set_services(uint64_t services)
    {
        services_ = services;
    }

    uint64_t timestamp() const
    {
        return timestamp_;
    }
    void set_timestamp(uint64_t timestamp)
    {
        timestamp_ = timestamp;
    }
    
    const NetAddr& addr_recv() const
    {
        return addr_recv_;
    }
    void set_addr_recv(const NetAddr& addr)
    {
        addr_recv_ = addr;
    }
    void set_addr_recv(NetAddr&& addr) noexcept
    {
        addr_recv_ = std::move(addr);
    }

    const NetAddr& addr_from() const
    {
        return addr_from_;
    }
    void set_addr_from(const NetAddr& addr)
    {
        addr_from_ = addr;
    }
    void set_addr_from(NetAddr&& addr) noexcept
    {
        addr_from_ = std::move(addr);
    }
    
    uint64_t nonce() const
    {
        return nonce_;
    }
    void set_nonce(uint64_t nonce)
    {
        nonce_ = nonce;
    }

    const std::string& user_agent() const
    {
        return user_agent_;
    }
    void set_user_agent(const std::string& agent)
    {
        user_agent_ = agent;
    }
    void set_user_agent(std::string&& agent) noexcept
    {
        user_agent_ = std::move(agent);
    }

    uint32_t start_height() const
    {
        return start_height_;
    }
    void set_start_height(uint32_t height)
    {
        start_height_ = height;
    }

    bool relay() const
    {
        return relay_;
    }
    void set_relay(bool relay)
    {
        relay_ = relay;
    }

private:
    uint32_t version_;
    uint64_t services_;
    uint64_t timestamp_;
    NetAddr addr_recv_;
    
    // Fields below require version ≥ 106
    NetAddr addr_from_;
    uint64_t nonce_;
    std::string user_agent_;
    uint32_t start_height_;
    
    // Fields below require version ≥ 70001
    bool relay_;
};

template <typename Stream>
void Version::Serialize(Stream& out) const
{
    Serializer<Stream> serial(out);
    
    serial.SerialWrite(version_);
    serial.SerialWrite(services_);
    serial.SerialWrite(timestamp_);
    serial.SerialWrite(addr_recv_);
    serial.SerialWrite(addr_from_);
    serial.SerialWrite(nonce_);
    serial.SerialWrite(addr_recv_);
    serial.SerialWrite(addr_from_);
    serial.SerialWrite(nonce_);
    serial.SerialWrite(user_agent_);
    serial.SerialWrite(start_height_);
    serial.SerialWrite(relay_);
}

template <typename Stream>
void Version::UnSerialize(Stream& in)
{

}

} // namespace message_types
} // namespace network
} // namespace btclite

#endif // BTCLITE_MESSAGE_VERSION_H
