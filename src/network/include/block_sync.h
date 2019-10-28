#ifndef BTCLITE_BLOCK_SYNC_H
#define BTCLITE_BLOCK_SYNC_H


#include <list>

#include "chain.h"
#include "network_address.h"


using NodeId = int64_t;

struct BlockReject {
    uint8_t reject_code = 0;
    std::string reject_reason;
    Hash256 block_hash;
};

/* Blocks that are in flight, and that are in the queue to be downloaded. */
struct QueuedBlock {
    Hash256 hash;
    const BlockIndex* index = nullptr;
    
    // Whether this block has validated headers at the time of request.
    bool has_validated_headers;
    
    // Optional, used for CMPCTBLOCK downloads
    //std::unique_ptr<PartiallyDownloadedBlock> partialBlock;
};

struct SyncBasicState {
    //! Whether we have a fully established connection.
    bool connected = false;
    
    //! Whether we've started headers synchronization with this peer.
    bool sync_started = false;
    
    //! Whether this peer should be disconnected and banned (unless whitelisted).
    bool should_ban = false;  
};

struct SyncStats {    
    //! Accumulated misbehaviour score for this peer.
    int misbehavior_score = 0;
    
    //! Length of current-streak of unconnecting headers announcements
    int unconnecting_headers_len = 0; 
    
    //! List of asynchronously-determined block rejections to notify this peer about.
    std::vector<BlockReject> rejects;
};

struct SyncTimeoutState {
    // A timeout used for checking whether our peer has sufficiently synced
    int64_t timeout = 0;
    
    // A header with the work we require on our peer's chain
    const BlockIndex *work_header = nullptr;
    
    // After timeout is reached, set to true after sending getheaders
    bool sent_getheaders = false;
    
    // Whether this peer is protected from disconnection due to a bad/slow chain
    bool protect = false;
};

struct SyncTimeState {
    //! When to potentially disconnect peer for stalling headers download
    int64_t headers_sync_timeout = 0;
    
    //! Since when we're stalling block download progress (in microseconds), or 0.
    int64_t stalling_since = 0;
    
    //! Time of last new block announcement
    int64_t last_block_announcement = 0;
    
    SyncTimeoutState timeout_state;
};

struct SyncProcessState {
    //! The best known block we know this peer has announced.
    const BlockIndex *best_known_block_index = nullptr;
    
    //! The last full block we both have.
    const BlockIndex *last_common_block_index = nullptr;
    
    //! The best header we have sent our peer.
    const BlockIndex *best_header_sent_index = nullptr;
    
    //! The hash of the last unknown block this peer has announced.
    Hash256 last_unknown_block_hash;
};

struct BlocksInFlight {
    using iterator = std::list<QueuedBlock>::iterator;
    
    std::list<QueuedBlock> list;
    
    //! When the first entry in BlocksInFlight started downloading. Don't care when BlocksInFlight is empty.
    int64_t downloading_since = 0;
    
    //int count;
    
    int valid_headers_count = 0;
};

struct DownloadState {
    //! Whether we consider this a preferred download peer.
    bool preferred_download = false;
    
    //! Whether this peer wants invs or headers (when possible) for block announcements.
    bool prefer_headers = false;
    
    //! Whether this peer wants invs or cmpctblocks (when possible) for block announcements.
    bool prefer_header_and_ids = false;
    
    /*
      * Whether this peer will send us cmpctblocks if we request them.
      * This is not used to gate request logic, as we really only care about supports_desired_cmpct_version_,
      * but is used as a flag to "lock in" the version of compact blocks (fWantsCmpctWitness) we send.
      */
    bool provides_header_and_ids = false;
    
    //! Whether this peer can give us witnesses
    bool is_witness = false;
    
    //! Whether this peer wants witnesses in cmpctblocks/blocktxns
    bool wants_cmpct_witness = false;
    
    /*
     * If we've announced kNodeWitness to this peer: whether the peer sends witnesses in cmpctblocks/blocktxns,
     * otherwise: whether this peer sends non-witnesses in cmpctblocks/blocktxns.
     */
    bool supports_desired_cmpct_version = false;
};

// Maintain validation-specific state about block sync
class BlockSyncState {
public:
    BlockSyncState(const btclite::network::NetAddr& addr, const std::string& addr_name)
        : node_addr_(addr), node_name_(addr_name) {}
    
    const btclite::network::NetAddr& node_addr() const
    {
        return node_addr_;
    }

    const SyncBasicState& basic_state() const
    {
        return basic_state_;
    }
    
    SyncBasicState *mutable_basic_state()
    {
        return &basic_state_;
    }
    
    const SyncStats& stats() const
    {
        return stats_;
    }
    
    SyncStats *mutable_stats()
    {
        return &stats_;
    }
    
    const SyncTimeState& time_state() const
    {
        return time_state_;
    }
    
    SyncTimeState *mutable_time_state()
    {
        return &time_state_;
    }
    
    const SyncProcessState& process_state() const
    {
        return process_state_;
    }
    
    SyncProcessState *mutable_process_state()
    {
        return &process_state_;
    }
    
    const BlocksInFlight& blocks_in_flight() const
    {
        return blocks_in_flight_;
    }
    
    BlocksInFlight *mutable_blocks_in_flight()
    {
        return &blocks_in_flight_;
    }
    
    const DownloadState& download_state() const
    {
        return download_state_;
    }
    
    DownloadState *mutable_download_state()
    {
        return &download_state_;
    }
    
private:    
    const btclite::network::NetAddr node_addr_;    
    const std::string node_name_;    
    SyncBasicState basic_state_;    
    SyncStats stats_;    
    SyncTimeState time_state_;    
    SyncProcessState process_state_;    
    BlocksInFlight blocks_in_flight_;    
    DownloadState download_state_;   
};

class BlockSync : Uncopyable {
public:
    using MapPeerSyncState = std::map<NodeId, BlockSyncState>;
    using MapBlockInFlight = std::map<Hash256, std::pair<NodeId, BlocksInFlight::iterator> >;
    
    //-------------------------------------------------------------------------
    void AddSyncState(NodeId id, const btclite::network::NetAddr& addr, const std::string& addr_name);    
    void EraseSyncState(NodeId id);    
    bool ShouldUpdateTime(NodeId id);
    void Misbehaving(NodeId id, int howmuch);
    void Clear();
    
    bool IsExist(NodeId id)
    {
        LOCK(cs_block_sync_);        
        return GetSyncState(id) ? true : false;
    }
    
    //-------------------------------------------------------------------------
    bool GetNodeAddr(NodeId id, btclite::network::NetAddr *out);
    
    //-------------------------------------------------------------------------
    // basic sync state
    bool GetConnected(NodeId id, bool *out);
    bool SetConnected(NodeId id, bool connected);    
    bool GetSyncStarted(NodeId id, bool *out);
    bool SetSyncStarted(NodeId id, bool sync_started);    
    bool GetShouldBan(NodeId id, bool *out);
    bool SetShouldBan(NodeId id, bool should_ban);
    
    //-------------------------------------------------------------------------
    // sync stats
    bool GetMisbehaviorScore(NodeId id, int *out);
    bool SetMisbehaviorScore(NodeId id, int misbehavior_score);
    bool GetUnconnectingHeadersLen(NodeId id, int *out);
    bool SetUnconnectingHeadersLen(NodeId id, int unconnecting_headers_len);
    bool GetRejects(NodeId id, std::vector<BlockReject> *out);
    bool AddBlockReject(NodeId id, const BlockReject& reject);
    bool ClearRejects(NodeId id);

    //-------------------------------------------------------------------------
    // sync timeout state
    bool GetTimeOut(NodeId id, int64_t *out);
    bool SetTimeout(NodeId id, int64_t timeout);
    const BlockIndex *GetWorkHeader(NodeId id);
    bool SetWorkHeader(NodeId id, const BlockIndex *index);
    bool GetSentGetheaders(NodeId id, bool *out);
    bool SetSentGetheaders(NodeId id, bool sent_getheaders);
    bool GetProtect(NodeId id, bool *out);
    bool SetProtect(NodeId id, bool protect);
    
    //-------------------------------------------------------------------------
    // sync time state
    bool GetHeadersSyncTimeout(NodeId id, int64_t *out);
    bool SetHeadersSyncTimeout(NodeId id, int64_t headers_sync_timeout);
    bool StallingSince(NodeId id, int64_t *out);
    bool SetStallingSince(NodeId id, int64_t stalling_since);
    bool GetLastBlockAnnouncement(NodeId id, int64_t *out);
    bool SetLastBlockAnnouncement(NodeId id, int64_t last_block_announcement);
    
    //-------------------------------------------------------------------------
    // sync process state
    const BlockIndex *GetBestKnownBlockIndex(NodeId id);
    bool SetBestKnownBlockIndex(NodeId id, const BlockIndex *index);
    const BlockIndex *GetLastCommonBlockIndex(NodeId id);
    bool SetLastCommonBlockIndex(NodeId id, const BlockIndex *index);
    const BlockIndex *GetBestHeaderSentIndex(NodeId id);
    bool SetBestHeaderSentIndex(NodeId id, const BlockIndex *index);
    bool GetLastUnknownBlockHash(NodeId id, Hash256 *out);
    bool SetLastUnknownBlockHash(NodeId id, const Hash256& last_unknown_block_hash);
    
    //-------------------------------------------------------------------------
    // blocks in flight
    bool ClearBlocksInFlight(NodeId id);
    bool GetDownloadingSince(NodeId id, int64_t *out);
    bool SetDownloadingSince(NodeId id, int64_t downloading_since);
    bool GetValidHeadersCount(NodeId id, int *out);
    bool SetValidHeadersCount(NodeId id, int valid_headers_count);
    
    //-------------------------------------------------------------------------
    // download state
    bool GetPreferredDownload(NodeId id, bool *out);
    bool SetPreferredDownload(NodeId id, bool preferred_download);
    bool GetPreferHeaders(NodeId id, bool *out);
    bool SetPreferHeaders(NodeId id, bool prefer_headers);
    bool GetPreferHeaderAndIds(NodeId id, bool *out);
    bool SetPreferHeaderAndIds(NodeId id, bool prefer_header_and_ids);
    bool GetProvidesHeaderAndIds(NodeId id, bool *out);
    bool SetProvidesHeaderAndIds(NodeId id, bool provides_header_and_ids);
    bool GetIsWitness(NodeId id, bool *out);
    bool SetIsWitness(NodeId id, bool is_witness);
    bool GetWantsCmpctWitness(NodeId id, bool *out);
    bool SetWantsCmpctWitness(NodeId id, bool wants_cmpct_witness);
    bool GetSupportsDesiredCmpctVersion(NodeId id, bool *out);
    bool SetSupportsDesiredCmpctVersion(NodeId id, bool supports_desired_cmpct_version);
    
private:
    mutable CriticalSection cs_block_sync_;
    
    // Number of nodes with SyncBasicState::sync_started.
    int sync_started_count_ = 0;
    
    // Number of preferable block download peers.
    int preferred_download_count_ = 0;
    
    // Number of peers from which we're downloading blocks.
    int validated_download_count_ = 0;

    // Number of outbound peers with SyncTimeoutState::protect.
    int protected_outbound_count_ = 0;
    
    MapPeerSyncState map_sync_state_;
    MapBlockInFlight map_block_in_flight_;
    
    // requires cs_block_sync_
    BlockSyncState *GetSyncState(NodeId id);
};

class SingletonBlockSync : Uncopyable {
public:
    static BlockSync& GetInstance()
    {
        static BlockSync block_sync;
        return block_sync;
    }
    
private:
    SingletonBlockSync() {}
};

template <typename Iter>
struct IterLess
{
    bool operator()(const Iter& a, const Iter& b) const
    {
        return &(*a) < &(*b);
    }
};

struct OrphanTx {
    using TxSharedPtr = std::shared_ptr<const Transaction>;
    
    // When modifying, adapt the copy of this definition in unit tests.
    TxSharedPtr tx;
    NodeId from_peer;
    int64_t time_expire;
};

class Orphans : Uncopyable {
public:
    using MapOrphanTxs = std::map<Hash256, OrphanTx>;
    using MapPrevOrphanTxs = std::map<OutPoint, std::set<MapOrphanTxs::iterator, IterLess<MapOrphanTxs::iterator> > >;
    
    bool AddOrphanTx(OrphanTx::TxSharedPtr tx, NodeId id);
    void EraseOrphansFor(NodeId id);
    uint32_t LimitOrphanTxSize(uint32_t max_orphans);
    
private:
    mutable CriticalSection cs_orphans_;
    MapOrphanTxs map_orphan_txs_;
    MapPrevOrphanTxs map_prev_orphan_txs_;
    
    int EraseOrphanTx(const Hash256& hash);
};

class SingletonOrphans : Uncopyable {
public:
    static Orphans& GetInstance()
    {
        static Orphans orphans;
        return orphans;
    }
    
private:
    SingletonOrphans() {}
};


#endif // BTCLITE_BLOCK_SYNC_H
