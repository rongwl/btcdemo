#include "protocol/reject.h"


namespace btclite {
namespace network {
namespace protocol {

const std::string Reject::kCommand = kMsgReject;

bool Reject::RecvHandler(std::shared_ptr<Node> src_node) const
{
    return true;
}

bool Reject::IsValid() const
{
    if (ccode_ != kRejectMalformed &&
        ccode_ != kRejectInvalid &&
        ccode_ != kRejectObsolete &&
        ccode_ != kRejectDuplicate &&
        ccode_ != kRejectNonstandard &&
        ccode_ != kRejectDust &&
        ccode_ != kRejectInsufficientfee &&
        ccode_ != kRejectCheckpoint)
        return false;
    
    return true;
}

void Reject::Clear()
{
    message_.clear();
    ccode_ = 0;
    reason_.clear();
    data_.Clear();
}

size_t Reject::SerializedSize() const
{
    return btclite::utility::serialize::VarIntSize(message_.size()) + message_.size() +
           btclite::utility::serialize::VarIntSize(reason_.size()) + reason_.size() +
           sizeof(ccode_) + data_.size();
}

bool Reject::operator==(const Reject& b) const
{
    return (message_ == b.message_ &&
            ccode_ == b.ccode_ &&
            reason_ == b.reason_ &&
            data_ == b.data_);
}

bool Reject::operator!=(const Reject& b) const
{
    return !(*this == b);
}

} // namespace protocol
} // namespace network
} // namespace btclite
