#include "protocol/send_headers.h"


namespace btclite {
namespace network {
namespace protocol {

bool SendHeaders::RecvHandler(std::shared_ptr<Node> src_node) const
{
    return true;
}

} // namespace protocol
} // namespace network
} // namespace btclite
