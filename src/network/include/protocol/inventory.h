#ifndef BTCLITE_PROTOCOL_INVENTORY_H
#define BTCLITE_PROTOCOL_INVENTORY_H


#include "message.h"
#include "inventory_vector.h"


namespace btclite {
namespace network {
namespace protocol {

class Inv {
public:
    bool RecvHandler(std::shared_ptr<Node> src_node) const;
    
    std::string Command() const
    {
        return msg_command::kMsgInv;
    }
    
    size_t SerializedSize() const;
    
    util::Hash256 GetHash() const
    {
        return crypto::GetHash(*this);
    }
    
    bool IsValid() const
    {
        return !inv_vects_.empty();
    }
    
    void Clear()
    {
        inv_vects_.clear();
    }
    
    //-------------------------------------------------------------------------
    bool operator==(const Inv& b) const
    {
        return (inv_vects_ == b.inv_vects_);
    }
    
    bool operator!=(const Inv& b) const
    {
        return !(*this == b);
    }
    
    //-------------------------------------------------------------------------
    template <typename Stream>
    void Serialize(Stream& out) const
    {
        util::Serializer<Stream> serializer(out);
        serializer.SerialWrite(inv_vects_);
    }
    
    template <typename Stream>
    void Deserialize(Stream& in)
    {
        util::Deserializer<Stream> deserializer(in);
        deserializer.SerialRead(&inv_vects_);
    }
    
    //-------------------------------------------------------------------------
    const std::vector<InvVect>& inv_vects() const
    {
        return inv_vects_;
    }
    
    std::vector<InvVect> *mutable_inv_vects()
    {
        return &inv_vects_;
    }
    
private:
    std::vector<InvVect> inv_vects_;
};

} // namespace protocol
} // namespace network
} // namespace btclite

#endif // BTCLITE_PROTOCOL_INVENTORY_H
