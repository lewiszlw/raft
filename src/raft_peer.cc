#include "raft.h"


namespace raft {


RaftPeer::RaftPeer(raft::Server server) : server_(server) {
    next_index_ = 1;
    match_index_ = 0;
    vote_granted_ = false;
}
RaftPeer::~RaftPeer() {
}

} // namespace raft