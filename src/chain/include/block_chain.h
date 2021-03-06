#ifndef BTCLITE_CHAIN_BLOCK_CHAIN_H
#define BTCLITE_CHAIN_BLOCK_CHAIN_H


#include "block_index.h"
#include "chain_state.h"
#include "chain/include/params.h"


namespace btclite {
namespace chain {

class BlockChain : util::Uncopyable {
public:
    BlockChain(const util::Configuration& config);
    
    //-------------------------------------------------------------------------
    bool Init();
    bool Start();
    void Interrupt();
    void Stop();
    
    //-------------------------------------------------------------------------    
    const ChainState& chain_state() const;    
    ChainState *mutable_chain_state();
    
private:
    const Params params_;
    ChainState chain_state_;
};

} // namespace chain
} // namespace btclite

#endif // BTCLITE_CHAIN_BLOCK_CHAIN_H
