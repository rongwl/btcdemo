#include "protocol/ping.h"

#include "msg_process.h"
#include "net.h"
#include "protocol/pong.h"
#include "random.h"


namespace btclite {
namespace network {
namespace protocol {

namespace private_ping {

bool Ping::RecvHandler(std::shared_ptr<Node> src_node, const Params& params) const
{
    if (src_node->protocol().version() >= kBip31Version)
    {
        // Echo the message back with the nonce. This allows for two useful features:
        //
        // 1) A remote node can quickly check if the connection is operational
        // 2) Remote nodes can measure the latency of the network thread. If this node
        //    is overloaded it won't respond to pings quickly and the remote node can
        //    avoid sending us more work, like chain download requests.
        //
        // The nonce stops the remote getting confused between different pings: without
        // it, if the remote node sends a ping once per second and this node takes 5
        // seconds to respond to each, the 5th ping the remote sends would appear to
        // return very quickly.
        Pong pong(nonce_);
        SendMsg(pong, params.msg_magic(), src_node);
    }
    
    return true;
}

void Ping::PingTimeoutCb(std::shared_ptr<Node> node, uint32_t magic)
{
    if (SingletonNetInterrupt::GetInstance())
        return;
    
    if (node->connection().connection_state() == NodeConnection::kDisconnected) {
        util::SingletonTimerMng::GetInstance().StopTimer(node->timers().ping_timer);
        return;
    }
    
    if (node->time().ping_time.ping_nonce_sent) {
        BTCLOG(LOG_LEVEL_WARNING) << "Peer " << node->id() << " ping timeout.";
        util::SingletonTimerMng::GetInstance().StopTimer(node->timers().ping_timer);
        node->mutable_connection()->set_connection_state(NodeConnection::kDisconnected);
        return;
    }
    
    // send ping
    protocol::Ping ping(util::RandUint64());
    if (node->protocol().version() < kBip31Version)
        ping.set_protocol_version(0);
    SendMsg(ping, magic, node);
}

} // namespace private_ping

} // namespace protocol
} // namespace network
} // namespace btclite
