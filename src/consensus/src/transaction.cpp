#include "transaction.h"

#include <botan/hash.h>
#include <numeric>


namespace btclite {
namespace consensus {

std::string TxIn::ToString() const
{
    std::stringstream ss;
    ss << "TxIn(" << prevout_.ToString() << ", ";
    if (prevout_.IsNull()) {
        ss << "scriptsig=" << util::EncodeHex(
               script_sig_.begin(), script_sig_.end());
    }
    else {
        ss << "scriptsig=" << util::EncodeHex(
               script_sig_.begin(), script_sig_.end()).substr(0, 24);
    }
    if (sequence_no_ != default_sequence_no) {
        ss << ", sequence_no=" << sequence_no_;
    }
    ss << ")";
    
    return ss.str();
}

std::string TxOut::ToString() const
{
    std::stringstream ss;
    ss << "TxOut(value=" << (value_ / kSatoshiPerBitcoin) << "."
       << std::setw(8) << std::setfill('0') << (value_ % kSatoshiPerBitcoin) << ", "
       << "scriptPubKey=" 
       << util::EncodeHex(script_pub_key_.begin(),
               script_pub_key_.end()) 
       << ")";
    return ss.str();
}

template <typename Stream>
void Transaction::Serialize(Stream& os, bool witness) const
{
    util::Serializer<Stream> serializer(os);
    serializer.SerialWrite(version_);
    serializer.SerialWrite(inputs_);
    serializer.SerialWrite(outputs_);
    serializer.SerialWrite(lock_time_);
}

template <typename Stream>
void Transaction::Deserialize(Stream& is, bool witness)
{
    util::Deserializer<Stream> deserializer(is);
    deserializer.SerialRead(&version_);
    deserializer.SerialRead(&inputs_);
    deserializer.SerialRead(&outputs_);
    deserializer.SerialRead(&lock_time_);
}

Transaction& Transaction::operator=(const Transaction& b)
{
    version_ = b.version_;
    inputs_ = b.inputs_;
    outputs_ = b.outputs_;
    lock_time_ = b.lock_time_;
    hash_cache_.fill(0);
    
    return *this;
}

Transaction& Transaction::operator=(Transaction&& b) noexcept
{
    if (this != &b) {
        version_ = b.version_;
        inputs_ = std::move(b.inputs_);
        outputs_ = std::move(b.outputs_);
        lock_time_ = std::move(b.lock_time_);
        hash_cache_.fill(0);
    }
    
    return *this;
}

uint64_t Transaction::OutputsAmount() const
{
    uint64_t amount = 0;
    for (auto output : outputs_) {
        amount += output.value();
        if (output.value() > kMaxSatoshiAmount || amount > kMaxSatoshiAmount)
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    
    return amount;
}

size_t Transaction::SerializedSize() const
{
    const auto ins = [](size_t size, const TxIn& input)
    {
        return size + input.SerializedSize();
    };
    const auto outs = [](size_t size, const TxOut& output)
    {
        return size + output.SerializedSize();
    };
    
    return sizeof(version_) + sizeof(lock_time_)
           + util::VarIntSize(inputs_.size())
           + std::accumulate(inputs_.begin(), inputs_.end(), size_t{0}, ins)
           + util::VarIntSize(outputs_.size())
           + std::accumulate(outputs_.begin(), outputs_.end(), size_t{0}, outs);
}

std::string Transaction::ToString() const
{
    std::stringstream ss;
    util::Hash256 hash = GetHash();
    ss << "Transaction(hash=" 
       << util::EncodeHex(hash.rbegin(), hash.rend()).substr(0,10) << ", "
       << "ver=" << version_ << ", "
       << "inputs.size=" << inputs_.size() << ", "
       << "outputs.size=" << outputs_.size() << ", "
       << "lock_time=" << lock_time_ << ")\n";
    for (const auto& tx_in : inputs_)
        ss << "    " << tx_in.ToString() << "\n";
    /*for (const auto& tx_in : inputs_)
        ss << "    " << tx_in.scriptWitness.ToString() << "\n";*/
    for (const auto& tx_out : outputs_)
        ss << "    " << tx_out.ToString() << "\n";
    
    return ss.str();
}

util::Hash256 Transaction::GetHash() const
{
    if (hash_cache_ == crypto::null_hash) {
        crypto::HashOStream hs;
        hs << *this;
        hash_cache_ = hs.DoubleSha256();
    }
    
    return hash_cache_;
}

util::Hash256 Transaction::GetWitnessHash() const
{
    return witness_hash_cache_;
}

} // namespace consensus
} // namespace btclite