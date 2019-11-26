#ifndef BTCLITE_MSG_PROCESS_H
#define BTCLITE_MSG_PROCESS_H

#include "hash.h"
#include "network/include/params.h"
#include "protocol/message.h"
#include "stream.h"


namespace btclite {
namespace network {
namespace msg_process{

btclite::network::protocol::MessageData *MsgDataFactory(const uint8_t *raw, 
                                                        const btclite::network::protocol::MessageHeader& header,
                                                        uint32_t protocol_version);
bool ParseMsg(std::shared_ptr<Node> src_node);

template <typename Message>
bool SendMsg(const Message& msg, std::shared_ptr<Node> dst_node)
{
    using namespace btclite::network::protocol;
    
    MemOstream ms;
        
    if (!dst_node->bev())
        return false;

    MessageHeader header(btclite::network::SingletonParams::GetInstance().msg_magic(),
                         msg.Command(), msg.SerializedSize(), msg.GetHash().GetLow32());
    ms << header << msg;
    
    if (ms.vec().size() != MessageHeader::kSize + msg.SerializedSize()) {
        BTCLOG(LOG_LEVEL_ERROR) << "Wrong serialized message size:" << ms.vec().size()
                                << ", correct size:" << MessageHeader::kSize + msg.SerializedSize() 
                                << ", message type:" << msg.Command();
        return false;
    }

    //bufferevent_lock(dst_node->mutable_bev());

    if (bufferevent_write(dst_node->mutable_bev(), ms.vec().data(), ms.vec().size())) {
        BTCLOG(LOG_LEVEL_ERROR) << "Writing message to bufferevent failed, peer:" << dst_node->id();
        return false;
    }

    //bufferevent_unlock(dst_node->mutable_bev());

    return true;
}

bool SendVersion(std::shared_ptr<Node> dst_node);
bool SendAddr(std::shared_ptr<Node> dst_node);

void UpdatePreferredDownload(std::shared_ptr<Node> node);

} // namespace msg_process
} // namespace network
} // namespace btclite

#endif // BTCLITE_MSG_PROCESS_H
