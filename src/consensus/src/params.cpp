#include "consensus/include/params.h"


namespace btclite {
namespace consensus {

void Bip9Params::UpdateVersionBitsParameters(Bip9Deployment::Deployment d, 
                                             int64_t start_time, int64_t timeout)
{

}

Params::Params(BtcNet btcnet)
{
    switch (btcnet) {
        case BtcNet::kMainNet :
        {
            CreateGenesisBlock(1231006505, 2083236893, 0x1d00ffff, 1,
                               50 * kSatoshiPerBitcoin);
            subsidy_halving_interval_ = 210000;
            break;
        }
        case BtcNet::kTestNet :
        {
            CreateGenesisBlock(1296688602, 414098458, 0x1d00ffff, 1, 
                               50 * kSatoshiPerBitcoin);
            subsidy_halving_interval_ = 210000;
            break;
        }
        case BtcNet::kRegTest :
        {
            CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 
                               50 * kSatoshiPerBitcoin);
            subsidy_halving_interval_ = 150;
            break;
        }
        default :
        {
            break;
        }
    }
}

const Block& Params::GenesisBlock() const
{
    return genesis_;
}

int Params::SubsideHalvingInterval() const
{
    return subsidy_halving_interval_;
}

const Bip9Params& Params::Bip9params() const
{
    return bip9_params_;
}

void Params::CreateGenesisBlock(const std::string& coinbase, const Script& output_script,
                                uint32_t time, uint32_t nonce, uint32_t bits, int32_t version,
                                uint64_t reward)
{
    Script script;    
    script.Push(0x1d00ffff);
    script.Push(ScriptInt(4));
    script.Push(std::vector<uint8_t>(coinbase.begin(), coinbase.end()));

    std::vector<TxIn> inputs;
    TxIn input(OutPoint(), script);
    inputs.push_back(input);

    std::vector<TxOut> outputs;
    outputs.push_back(TxOut(reward, output_script));

    std::vector<Transaction> transactions;
    Transaction tx_new(1, inputs, outputs, 0);
    transactions.push_back(tx_new);

    genesis_.set_transactions(transactions);
    BlockHeader header(version, util::Hash256(), genesis_.ComputeMerkleRoot(), time, bits, nonce);    
    genesis_.set_header(header);
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * Block(hash=0000000000, ver=0x00000001, hashPrevBlock=0000000000, hashMerkleRoot=4a5e1e4baa, nTime=1231006505, bits=1d00ffff, nNonce=2083236893, tx.size=1)
 *   Transaction(hash=4a5e1e4baa, ver=1, inputs.size=1, outputs.size=1, lock_time=0)
 *     TxIn(OutPoint(0000000000, 4294967295), scriptsig 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     TxOut(value=50.00000000, scriptPubKey=4104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac)
 *   vMerkleTree: 4a5e1e
 */
void Params::CreateGenesisBlock(uint32_t time, uint32_t nonce, uint32_t bits, 
                                int32_t version, uint64_t reward)
{
    const std::string coinbase = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    Script output_script;
    output_script.Push(
        util::DecodeHex(
            "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f"));
    output_script.Push(Opcode::OP_CHECKSIG);

    CreateGenesisBlock(coinbase, output_script, time, 
                       nonce, bits, version, reward);
}

} // namespace consensus
} // namespace btclite
