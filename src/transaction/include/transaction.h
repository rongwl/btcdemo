#ifndef BTCLITE_TRANSACTION_H
#define BTCLITE_TRANSACTION_H

#include "hash.h"
#include "script.h"
#include "serialize.h"
#include "tinyformat.h"

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class OutPoint {
public:
	OutPoint()
		: index_(UINT32_MAX) {}
	OutPoint(const Hash256& hash, uint32_t index)
		: hash_(hash), index_(index) {}
	OutPoint(Hash256&& hash, uint32_t index) noexcept
		: hash_(std::move(hash)), index_(index) {}
	OutPoint(const OutPoint& op)
		: hash_(op.hash_), index_(op.index_) {}
	OutPoint(OutPoint&& op) noexcept
		: hash_(std::move(op.hash_)), index_(op.index_) {}
	
	//-------------------------------------------------------------------------
	void SetNull()
	{
		hash_.SetNull();
		index_ = UINT32_MAX;
	}
	bool IsNull() const
	{
		return (hash_.IsNull() && index_ == UINT32_MAX);
	}
	std::size_t Size() const
	{
		return hash_.size() + sizeof(index_);
	}
	std::string ToString() const
	{
		return strprintf("OutPoint(%s, %u)", hash_.ToString().substr(0,10), index_);
	}
	
	//-------------------------------------------------------------------------
	friend bool operator==(const OutPoint& a, const OutPoint& b)
	{
		return (a.hash_ == b.hash_ && a.index_ == b.index_);
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
		hash_ = b.hash_;
		index_ = b.index_;
		return *this;
	}
	OutPoint& operator=(OutPoint&& b) noexcept
	{
		if (this != &b) {
			hash_ = std::move(b.hash_);
			index_ = b.index_;
		}
		return *this;
	}
	
	//-------------------------------------------------------------------------
	template <typename SType>
	void Serialize(SType& os) const
	{
		Serializer<SType> serial(os);
		serial.SerialWrite(hash_);
		serial.SerialWrite(index_);
	}
	template <typename SType>
	void UnSerialize(SType& is)
	{
		Serializer<SType> serial(is);
		serial.SerialRead(&hash_);
		serial.SerialRead(&index_);
	}
	
	//-------------------------------------------------------------------------
	const Hash256& prevhash()
	{
		return hash_;
	}
private:
	Hash256 hash_;
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
	TxIn(const OutPoint& prevout, const Script& script_sig, uint32_t sequence_no=default_sequence_no)
		: prevout_(prevout), script_sig_(script_sig), sequence_no_(sequence_no) {}
	TxIn(OutPoint&& prevout, Script&& script_sig, uint32_t sequence_no=default_sequence_no) noexcept
		: prevout_(std::move(prevout)), script_sig_(std::move(script_sig)), sequence_no_(sequence_no) {}
	TxIn(const TxIn& input)
		: prevout_(input.prevout_), script_sig_(input.script_sig_), sequence_no_(input.sequence_no_) {}
	TxIn(TxIn&& input) noexcept
		: prevout_(std::move(input.prevout_)), script_sig_(std::move(input.script_sig_)), sequence_no_(input.sequence_no_) {}
	
	//-------------------------------------------------------------------------
	template <typename SType>
	void Serialize(SType& os) const
	{
		Serializer<SType> serial(os);
		serial.SerialWrite(prevout_);
		serial.SerialWrite(script_sig_);
		serial.SerialWrite(sequence_no_);
	}
	template <typename SType>
	void UnSerialize(SType& is)
	{
		Serializer<SType> serial(is);
		serial.SerialRead(&prevout_);
		serial.SerialRead(&script_sig_);
		serial.SerialRead(&sequence_no_);
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
	std::size_t Size(bool serialized = false) const
	{
		return prevout_.Size() + script_sig_.Size(serialized) + sizeof(sequence_no_);
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
	
private:
	OutPoint prevout_;
	Script script_sig_;
	uint32_t sequence_no_;
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
	void SetNull()
	{
		value_ = null_value;
		script_pub_key_.clear();
	}
	bool IsNull()
	{
		return value_ == null_value;
	}
	std::string ToString() const
	{
		return strprintf("TxOut(value=%d.%08d, scriptPubKey=%s)", 
						 value_ / satoshi_per_bitcoin, value_ % satoshi_per_bitcoin, 
						 HexEncode(script_pub_key_.begin(), script_pub_key_.end()).substr(0, 30));
	}
	std::size_t Size(bool serialized = false) const
	{
		return script_pub_key_.Size(serialized) + sizeof(value_);
	}
	
	//-------------------------------------------------------------------------
	template <typename SType>
	void Serialize(SType& os) const
	{
		Serializer<SType> serial(os);
		serial.SerialWrite(value_);
		serial.SerialWrite(script_pub_key_);
	}
	template <typename SType>
	void UnSerialize(SType& is)
	{
		Serializer<SType> serial(is);
		serial.SerialRead(&value_);
		serial.SerialRead(&script_pub_key_);
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
	
private:
	uint64_t value_;
	Script script_pub_key_;

	static constexpr uint64_t null_value = UINT64_MAX;
};

class Transaction {
public:
	Transaction()
		: version_(default_version), inputs_(), outputs_(), lock_time_(0), hash_() {}
	Transaction(uint32_t version, const std::vector<TxIn>& inputs,
				const std::vector<TxOut>& outputs, uint32_t lock_time)
		: version_(version), inputs_(inputs), outputs_(outputs), lock_time_(lock_time)
	{
		Hash();
	}
	Transaction(uint32_t version, std::vector<TxIn>&& inputs,
				std::vector<TxOut>&& outputs, uint32_t lock_time) noexcept
		: version_(version), inputs_(std::move(inputs)),
		  outputs_(std::move(outputs)), lock_time_(lock_time)
	{
		Hash();
	}
	Transaction(const Transaction& t)
		: version_(t.version_), inputs_(t.inputs_), outputs_(t.outputs_), lock_time_(t.lock_time_) 
	{
		Hash();
	}
	Transaction(Transaction&& t) noexcept
		: version_(t.version_), inputs_(std::move(t.inputs_)),
		  outputs_(std::move(t.outputs_)), lock_time_(t.lock_time_)
	{
		Hash();
	}
	
	//-------------------------------------------------------------------------
	template <typename SType> void Serialize(SType&) const;
	template <typename SType> void UnSerialize(SType&);
	
	//-------------------------------------------------------------------------
	bool operator==(const Transaction& b) const
	{
		return hash_ == b.hash_;
	}
	bool operator!=(const Transaction& b) const
	{
		return !(*this == b);
	}
	Transaction& operator=(const Transaction& b)
	{
		version_ = b.version_;
		inputs_ = b.inputs_;
		outputs_ = b.outputs_;
		lock_time_ = b.lock_time_;
		return *this;
	}
	Transaction& operator=(Transaction&& b) noexcept
	{
		if (this != &b) {
			version_ = b.version_;
			inputs_ = std::move(b.inputs_);
			outputs_ = std::move(b.outputs_);
			lock_time_ = std::move(b.lock_time_);
		}
		return *this;
	}
	
	//-------------------------------------------------------------------------
	const Hash256& HashCache() const
	{
		return hash_;
	}
	const Hash256& Hash();
	
	//-------------------------------------------------------------------------
	bool IsNull() const
	{
		return inputs_.empty() && outputs_.empty();
	}
	bool IsCoinBase() const
	{
		return (inputs_.size() == 1 && inputs_[0].prevout().IsNull());
	}
	std::size_t Size(bool) const;
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
	}
	
	const std::vector<TxIn>& inputs() const
	{
		return inputs_;
	}
	void set_inputs(const std::vector<TxIn>& inputs)
	{
		inputs_ = inputs;
	}
	void set_inputs(std::vector<TxIn>&& inputs)
	{
		inputs_ = std::move(inputs);
	}
	
	const std::vector<TxOut>& outputs() const
	{
		return outputs_;
	}
	void set_outputs(const std::vector<TxOut>& outputs)
	{
		outputs_ = outputs;
	}
	void set_outputs(std::vector<TxOut>&& outputs)
	{
		outputs_ = std::move(outputs);
	}
	
	uint32_t lock_time() const
	{
		return lock_time_;
	}
	void set_lockTime(uint32_t t)
	{
		lock_time_ = t;
	}
	
private:
	uint32_t version_;
	std::vector<TxIn> inputs_;
	std::vector<TxOut> outputs_;
	uint32_t lock_time_;
	
	Hash256 hash_;
	
	// Default transaction version.
    static constexpr uint32_t default_version = 2;
};

#endif // BTCLITE_TRANSACTION_H
