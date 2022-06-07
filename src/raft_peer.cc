#include "raft.h"


namespace raft {


RaftPeer::RaftPeer(raft::Server server) : server_(server) {
    next_index_ = 1;
    match_index_ = 0;
    vote_granted_ = false;
    is_leader_ = false;
}
RaftPeer::~RaftPeer() {
}

RaftPeerManager::RaftPeerManager() {
}
RaftPeerManager::~RaftPeerManager() {
}
void RaftPeerManager::AddPeers(std::vector<raft::Server> servers, uint64_t next_index) {
    for (auto it = servers.begin(); it != servers.end(); ++it) {
        RaftPeer peer(*it);
        // 已复制到该服务器的最高日志条目的索引，从0开始单调递增
        peer.match_index_ = 0;
        // 发送到该服务器的下一个日志条目的索引，初始值为领导人最后的日志条目的索引+1
        peer.next_index_ = next_index;
        peer.vote_granted_ = false;
        peers_.push_back(peer);
    }
}
RaftPeer* RaftPeerManager::GetLeader() {
    for (auto it = peers_.begin(); it != peers_.end(); ++it) {
        if (it->is_leader_) {
            return &(*it);
        }
    }
    return nullptr;
}
std::vector<RaftPeer>& RaftPeerManager::GetPeers() {
    return peers_;
}


} // namespace raft