#ifndef BTCLITE_CONSENSUS_TRANSACTION_H
#define BTCLITE_CONSENSUS_TRANSACTION_H

#include "hash.h"
#include "script.h"
#include "script_witness.h"
#include "serialize.h"


namespace btclite {
namespace consensus {

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class OutPoint {
public:
    OutPoint()
        : index_(std::numeric_limits<uint32_t>::max()) {}
    
    OutPoint(const util::Hash256& hash, uint32_t index)
        : prev_hash_(hash), index_(index) {}
    OutPoint(util::Hash256&& hash, uint32_t index) noexcept
        : prev_hash_(std::move(hash)), index_(index) {}
    
    OutPoint(const OutPoint& op)
        : prev_hash_(op.prev_hash_), index_(op.index_) {}
    OutPoint(OutPoint&& op) noexcept
        : prev_hash_(std::move(op.prev_hash_)), index_(op.index_) {}
    
    //-------------------------------------------------------------------------
    void Clear()
    {
        prev_hash_.fill(0);
        index_ = std::numeric_limits<uint32_t>::max();
    }
    
    bool IsNull() const
    {
        return (prev_hash_ == crypto::null_hash &&
                index_ == std::numeric_limits<uint32_t>::max());
    }
    
    size_t Size() const
    {
        return prev_hash_.size() + sizeof(index_);
    }
    
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "OutPoint(" 
           << util::EncodeHex(prev_hash_.rbegin(), prev_hash_.rend()).substr(0, 10) 
           << ", " << index_ << ")";
        return ss.str();
    }
    
    //-------------------------------------------------------------------------
    friend bool operator==(const OutPoint& a, const OutPoint& b)
    {
        return (a.prev_hash_ == b.prev_hash_ && a.index_ == b.index_);
    }
    
    friend bool operator!=(const OutPoint& a, const OutPoint& b)
    {
        return !(a == b);
    }
    
    friend bool operator<(const OutPoint& a, const OutPoint& b)
    {
        return (a < b || (a == b && a.index_ < b.index_));
    }
    
    friend bool operator>(const OutPoint& a, const OutPoint& b)
    {
        return (a > b || (a == b && a.index_ > b.index_));
    }
    
    OutPoint& operator=(const OutPoint& b)
    {
        prev_hash_ = b.prev_hash_;
        index_ = b.index_;
        return *this;
    }
    
    OutPoint& operator=(OutPoint&& b) noexcept
    {
        if (this != &b) {
            prev_hash_ = std::move(b.prev_hash_);
            index_ = b.index_;
        }
        return *this;
    }
    
    //-------------------------------------------------------------------------
    template <typename Stream>
    void Serialize(Stream& os) const
    {
        util::Serializer<Stream> serializer(os);
        serializer.SerialWrite(prev_hash_);
        serializer.SerialWrite(index_);
    }
    
    template <typename Stream>
    void Deserialize(Stream& is)
    {
        util::Deserializer<Stream> deserializer(is);
        deserializer.SerialRead(&prev_hash_);
        deserializer.SerialRead(&index_);
    }
    
    //-------------------------------------------------------------------------
    const util::Hash256& prev_hash() const
    {
        return prev_hash_;
    }
    void set_prevHash(const util::Hash256& hash)
    {
        prev_hash_ = hash;
    }
    void set_prevHash(util::Hash256&& hash)
    {
        prev_hash_ = std::move(hash);
    }
    
    uint32_t index() const
    {
        return index_;
    }
    void set_index(uint32_t index)
    {
        index_ = index;
    }
    
private:
    util::Hash256 prev_hash_;
    uint32_t index_;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class TxIn {
public:
    /* Setting nSequence to this value for every input in a transaction
    * disables nLockTime. */
    static constexpr uint32_t default_sequence_no = 0xffffffff;

    //-------------------------------------------------------------------------
    TxIn()
        : sequence_no_(default_sequence_no) {}
    
    TxIn(const OutPoint& prevout, const Script& script_sig, uint32_t sequence_no=default_sequence_no,
         const ScriptWitness& script_witness = ScriptWitness())
        : prevout_(prevout), script_sig_(script_sig),
          sequence_no_(sequence_no), script_witness_(script_witness) {}
    TxIn(OutPoint&& prevout, Script&& script_sig, uint32_t sequence_no=default_sequence_no,
         ScriptWitness&& script_witness = ScriptWitness()) noexcept
        : prevout_(std::move(prevout)), script_sig_(std::move(script_sig)),
          sequence_no_(sequence_no), script_witness_(std::move(script_witness)) {}
    
    TxIn(const TxIn& input)
        : prevout_(input.prevout_), script_sig_(input.script_sig_), 
          sequence_no_(input.sequence_no_), script_witness_(input.script_witness_) {}
    TxIn(TxIn&& input) noexcept
        : prevout_(std::move(input.prevout_)), script_sig_(std::move(input.script_sig_)),
          sequence_no_(input.sequence_no_), script_witness_(std::move(input.script_witness_)) {}
    
    //-------------------------------------------------------------------------
    template <typename Stream>
    void Serialize(Stream& os) const
    {
        util::Serializer<Stream> serializer(os);
        serializer.SerialWrite(prevout_);
        serializer.SerialWrite(script_sig_);
        serializer.SerialWrite(sequence_no_);
    }
    
    template <typename Stream>
    void Deserialize(Stream& is)
    {
        util::Deserializer<Stream> deserializer(is);
        deserializer.SerialRead(&prevout_);
        deserializer.SerialRead(&script_sig_);
        deserializer.SerialRead(&sequence_no_);
    }
    
    //-------------------------------------------------------------------------
    bool operator==(const TxIn& b) const
    {
        return (prevout_ == b.prevout_ &&
                script_sig_ == b.script_sig_ &&
                sequence_no_ == b.sequence_no_);
    }
    
    bool operator!=(const TxIn& b) const
    {
        return !(*this == b);
    }
    
    TxIn& operator=(const TxIn& b)
    {
        prevout_ = b.prevout_;
        script_sig_ = b.script_sig_;
        sequence_no_ = b.sequence_no_;
        return *this;
    }
    
    TxIn& operator=(TxIn&& b) noexcept
    {
        if (this != &b) {
            prevout_ = std::move(b.prevout_);
            script_sig_ = std::move(b.script_sig_);
            sequence_no_ = b.sequence_no_;
        }
        return *this;
    }
    
    //-------------------------------------------------------------------------
    std::string ToString() const;
    
    size_t SerializedSize() const
    {
        return prevout_.Size() + script_sig_.SerializedSize() + sizeof(sequence_no_);
    }
    
    bool HasWitness() const
    {
        return false;
    }
    
    //-------------------------------------------------------------------------
    const OutPoint& prevout() const
    {
        return prevout_;
    }
    void set_prevout(const OutPoint& prevout)
    {
        prevout_ = prevout;
    }
    void set_prevout(const OutPoint&& prevout)
    {
        prevout_ = std::move(prevout);
    }
    
    const Script& script_sig() const
    {
        return script_sig_;
    }
    void set_scriptSig(const Script& script)
    {
        script_sig_ = script;
    }
    void set_scriptSig(const Script&& script)
    {
        script_sig_ = std::move(script);
    }
    
    uint32_t sequence_no() const
    {
        return sequence_no_;
    }
    void set_sequenceNo(uint32_t sequence)
    {
        sequence_no_ = sequence;
    }
    
    const ScriptWitness& script_witness() const
    {
        return script_witness_;
    }
    void set_scriptWitness(const ScriptWitness& script_witness)
    {
        script_witness_ = script_witness;
    }
    void set_scriptWitness(ScriptWitness&& script_witness)
    {
        script_witness_ = std::move(script_witness);
    }
    
private:
    OutPoint prevout_;
    Script script_sig_;
    uint32_t sequence_no_;
    ScriptWitness script_witness_;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class TxOut {
public:
    TxOut()
        : value_(null_value)
    {
        script_pub_key_.clear();
    }
    
    TxOut(uint64_t value, const Script& script)
        : value_(value), script_pub_key_(script) {}
    TxOut(uint64_t value, Script&& script) noexcept
        : value_(value), script_pub_key_(std::move(script)) {}
    
    TxOut(const TxOut& output)
        : value_(output.value_), script_pub_key_(output.script_pub_key_) {}
    TxOut(TxOut&& output) noexcept
        : value_(output.value_), script_pub_key_(std::move(output.script_pub_key_)) {}
    
    //-------------------------------------------------------------------------
    void Clear()
    {
        value_ = null_value;
        script_pub_key_.clear();
    }
    
    bool IsNull()
    {
        return value_ == null_value;
    }
    
    std::string ToString() const;
    
    size_t SerializedSize() const
    {
        return script_pub_key_.SerializedSize() + sizeof(value_);
    }
    
    //-------------------------------------------------------------------------
    template <typename Stream>
    void Serialize(Stream& os) const
    {
        util::Serializer<Stream> serializer(os);
        serializer.SerialWrite(value_);
        serializer.SerialWrite(script_pub_key_);
    }
    template <typename Stream>
    void Deserialize(Stream& is)
    {
        util::Deserializer<Stream> deserializer(is);
        deserializer.SerialRead(&value_);
        deserializer.SerialRead(&script_pub_key_);
    }
    
    //-------------------------------------------------------------------------
    bool operator==(const TxOut& b) const
    {
        return (value_ == b.value_ &&
                script_pub_key_ == b.script_pub_key_);
    }
    bool operator!=(const TxOut& b) const
    {
        return !(*this == b);
    }
    TxOut& operator=(const TxOut& b)
    {
        value_ = b.value_;
        script_pub_key_ = b.script_pub_key_;
        return *this;
    }
    TxOut& operator=(TxOut&& b) noexcept
    {
        if (this != &b) {
            value_ = b.value_;
            script_pub_key_ = std::move(b.script_pub_key_);
        }
        return *this;
    }
    
    //-------------------------------------------------------------------------
    uint64_t value() const
    {
        return value_;
    }
    void set_value(uint64_t value)
    {
        value_ = value;
    }
    
    const Script& script_pub_key() const
    {
        return script_pub_key_;
    }
    void set_scriptPubKey(const Script& script)
    {
        script_pub_key_ = script;
    }
    void set_scriptPubKey(Script&& script)
    {
        script_pub_key_ = std::move(script);
    }
    
private:
    uint64_t value_;
    Script script_pub_key_;

    static constexpr uint64_t null_value = std::numeric_limits<uint64_t>::max();
};

class Transaction {
public:
    Transaction()
        : version_(default_version), inputs_(), outputs_(), lock_time_(0), hash_cache_() {}
    Transaction(uint32_t version, const std::vector<TxIn>& inputs,
                const std::vector<TxOut>& outputs, uint32_t lock_time)
        : version_(version), inputs_(inputs), outputs_(outputs), lock_time_(lock_time)
    {
        GetHash();
    }
    Transaction(uint32_t version, std::vector<TxIn>&& inputs,
                std::vector<TxOut>&& outputs, uint32_t lock_time) noexcept
        : version_(version), inputs_(std::move(inputs)),
          outputs_(std::move(outputs)), lock_time_(lock_time)
    {
        GetHash();
    }
    Transaction(const Transaction& t)
        : version_(t.version_), inputs_(t.inputs_), outputs_(t.outputs_),
          lock_time_(t.lock_time_)
    {
        GetHash();
    }
    Transaction(Transaction&& t) noexcept
        : version_(t.version_), inputs_(std::move(t.inputs_)), outputs_(std::move(t.outputs_)),
          lock_time_(t.lock_time_)
    {
        GetHash();
    }
    
    //-------------------------------------------------------------------------
    template <typename Stream> void Serialize(Stream& os, bool witness = false) const;
    template <typename Stream> void Deserialize(Stream& is, bool witness = false);
    
    //-------------------------------------------------------------------------
    bool operator==(const Transaction& b) const
    {
        return this->GetHash() == b.GetHash();
    }
    bool operator!=(const Transaction& b) const
    {
        return !(*this == b);
    }
    Transaction& operator=(const Transaction& b);
    Transaction& operator=(Transaction&& b) noexcept;
    
    //-------------------------------------------------------------------------
    util::Hash256 GetHash() const;
    util::Hash256 GetWitnessHash() const;
    
    //-------------------------------------------------------------------------
    bool IsNull() const
    {
        return inputs_.empty() && outputs_.empty();
    }
    
    bool IsCoinBase() const
    {
        return (inputs_.size() == 1 && inputs_[0].prevout().IsNull());
    }
    
    size_t SerializedSize() const;
    uint64_t OutputsAmount() const;
    std::string ToString() const;
    
    //-------------------------------------------------------------------------
    uint32_t version() const
    {
        return version_;
    }
    void set_version(uint32_t v)
    {
        version_ = v;
        hash_cache_.fill(0);
    }
    
    const std::vector<TxIn>& inputs() const
    {
        return inputs_;
    }
    void set_inputs(const std::vector<TxIn>& inputs)
    {
        inputs_ = inputs;
        hash_cache_.fill(0);
    }
    void set_inputs(std::vector<TxIn>&& inputs)
    {
        inputs_ = std::move(inputs);
        hash_cache_.fill(0);
    }
    
    const std::vector<TxOut>& outputs() const
    {
        return outputs_;
    }
    void set_outputs(const std::vector<TxOut>& outputs)
    {
        outputs_ = outputs;
        hash_cache_.fill(0);
    }
    void set_outputs(std::vector<TxOut>&& outputs)
    {
        outputs_ = std::move(outputs);
        hash_cache_.fill(0);
    }
    
    uint32_t lock_time() const
    {
        return lock_time_;
    }
    void set_lockTime(uint32_t t)
    {
        lock_time_ = t;
        hash_cache_.fill(0);
    }
    
private:    
    uint32_t version_;
    std::vector<TxIn> inputs_;
    std::vector<TxOut> outputs_;
    uint32_t lock_time_;
    
    mutable util::Hash256 hash_cache_;
    mutable util::Hash256 witness_hash_cache_;
    
    // Default transaction version.
    static constexpr uint32_t default_version = 2;
};

} // namespace consensus
} // namespace btclite

#endif // BTCLITE_CONSENSUS_TRANSACTION_H