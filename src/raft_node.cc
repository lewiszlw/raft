#include "raft.h"
#include <unistd.h>


namespace raft {

RaftNode::RaftNode(raft::Server local_server, std::vector<raft::Server> peer_servers) {
    local_server_ = local_server;
    for (size_t i = 0; i < peer_servers.size(); i++) {
        RaftPeer peer(peer_servers[i]);
        peers_.push_back(peer);
    }

    // 初始化节点状态为Follower
    state_ = RaftNodeState::FOLLOWER;
    voted_for_ = 0;
    current_term_ = 0;

    // TODO 加载snapshot

    // 初始化rpc客户端和服务端
    raft_consensus_service_client_ = new RaftConsensusServiceClient();
    raft_consensus_service_impl_ = new RaftConsensusServiceImpl(std::bind(&RaftNode::HandleAppendEntries, this, std::placeholders::_1, std::placeholders::_2),
                                                                std::bind(&RaftNode::HandleRequestVote, this, std::placeholders::_1, std::placeholders::_2),
                                                                std::bind(&RaftNode::HandleInstallSnapshot, this, std::placeholders::_1, std::placeholders::_2));
    raft_consensus_service_server_ = new RaftConsensusServiceServer(local_server_, raft_consensus_service_impl_);

    // 初始化计时器
    election_timer_ = new RaftTimer();
    heartbeat_timer_ = new RaftTimer();
    snapshot_timer_ = new RaftTimer();


    // TODO 临时测试
    current_term_ = local_server.server_id();
}
RaftNode::~RaftNode() {
}
void RaftNode::Start() {
    // 启动 Raft 节点
    // 启动 Raft 节点的 RPC 服务
    raft_consensus_service_server_->Start();

    // TODO 测试
    sleep(15);
    // AppendEntries();

    // 启动 Raft 节点的定时器
    election_timer_->Schedule(10000, [this](){ HandleElectionTimeout(); });
    heartbeat_timer_->Schedule(5000, [this]() { HandleHeartbeatTimeout(); });
    snapshot_timer_->Schedule(15000, [this]() { HandleSnapshotTimeout(); });

    raft_consensus_service_server_->grpc_server_->Wait();

    // 启动 Raft 节点的状态机
}
void RaftNode::AppendEntries() {
    std::cout << "RaftNode::AppendEntries" << std::endl;
    // TODO
    for (size_t i = 0; i < peers_.size(); i++) {
        raft::AppendEntriesReq request;
        request.set_leaderid(local_server_.server_id());
        request.set_term(current_term_);
        raft_consensus_service_client_->AppendEntries(request, peers_[i].server_);
    }
}

void RaftNode::RequestVote() {
    std::cout << "RaftNode::RequestVote" << std::endl;
    uint32_t vote_granted_count = 0;
    // TODO 并行地向所有的其他节点请求投票
    for (size_t i = 0; i < peers_.size(); i++) {
        RequestVoteReq request;
        // TODO 
        request.set_candidateid(local_server_.server_id());
        request.set_term(current_term_);
        RequestVoteResp response = raft_consensus_service_client_->RequestVote(request, peers_[i].server_);
        if (response.term() > current_term_) {
            state_ = RaftNodeState::FOLLOWER;
        }
        if (response.votegranted()) {
            vote_granted_count++;
            peers_[i].vote_granted_ = true;
        }
    }
    // 获得多数投票后成为leader
    if (vote_granted_count > peers_.size() / 2) {
        std::cout << "Become leader" << std::endl;
        state_ = RaftNodeState::LEADER;
        // 成为leader后立刻发送心跳
        AppendEntries();
    }

}
void RaftNode::InstallSnapshot() {
    std::cout << "RaftNode::InstallSnapshot" << std::endl;
}

void RaftNode::HandleAppendEntries(const raft::AppendEntriesReq* request, raft::AppendEntriesResp* response) {
    std::cout << "Receive AppendEntries from leader " << request->leaderid() << std::endl;
    if (state_ == RaftNodeState::FOLLOWER) {
        // 如果是follower，重置选举计时器
        election_timer_->Reset(10000);

    } else if (state_ == RaftNodeState::CANDIDATE) {
        // 如果是candidate，leader任期不小于自己的任期，则承认leader地位，转为follower
        if (request->term() >= current_term_) {
            state_ = RaftNodeState::FOLLOWER;
        } else {
            // 如果leader任期小于自己的任期，则拒绝承认leader并且继续保持候选人状态
            std::cout << "Refuse AppendEntries from leader as candidate " << request->leaderid() << std::endl;
        }

    } else if (state_ == RaftNodeState::LEADER) {
        // 如果是leader
        state_ = RaftNodeState::FOLLOWER;
    }
}

void RaftNode::HandleRequestVote(const raft::RequestVoteReq* request, raft::RequestVoteResp* response) {
    std::cout << "Receive RequestVote from candidate " << request->candidateid() << std::endl;

    // 如果candidate任期小于自己的任期，则拒绝投票
    if (request->term() < current_term_) {
        response->set_term(current_term_);
        response->set_votegranted(false);
    }

    // 如果未投票或已投票给candidate，则本次投票给candidate
    if (voted_for_ == 0 || voted_for_ == request->candidateid()) {
        std::cout << "Vote for " << request->candidateid() << std::endl;
        response->set_votegranted(true);
        response->set_term(current_term_);
        voted_for_ = request->candidateid();
    }
}

void RaftNode::HandleInstallSnapshot(const raft::InstallSnapshotReq* request, raft::InstallSnapshotResp* response) {
    std::cout << "RaftNode::HandleInstallSnapshot " << request->leaderid() << std::endl;
}

void RaftNode::HandleHeartbeatTimeout() {
    std::cout << "RaftNode::HandleHeartbeatTimeout " << std::chrono::system_clock::now().time_since_epoch().count() / 1000 << std::endl;

    if (state_ == RaftNodeState::LEADER) {
        // leader发送心跳
        AppendEntries();
        // 重置心跳计时器
        heartbeat_timer_->Reset(5000);

    } else if (state_ == RaftNodeState::FOLLOWER) {
    } else if (state_ == RaftNodeState::CANDIDATE) {
    }
}

void RaftNode::HandleElectionTimeout() {
    std::cout << "RaftNode::HandleElectionTimeout " << std::chrono::system_clock::now().time_since_epoch().count() / 1000 << std::endl;

    if (state_ == RaftNodeState::FOLLOWER) {
        // follower发起选举
        // 状态转为candidate
        state_ = RaftNodeState::CANDIDATE;
        // 任期号加1
        current_term_++;
        RequestVote();

        // 重置选举计时器
        std::cout << "Reset election timer" << std::endl;
        election_timer_->Reset(10000);   // 随机时间

    } else if (state_ == RaftNodeState::CANDIDATE) {
        // candidate再次发起选举
        // 任期号加1
        current_term_++;
        RequestVote();

        // 重置选举计时器
        std::cout << "Reset election timer" << std::endl;
        election_timer_->Reset(10000);   // 随机时间

    } else if (state_ == RaftNodeState::LEADER) {
    }
}

void RaftNode::HandleSnapshotTimeout() {
    // std::cout << "RaftNode::HandleSnapshotTimeout " << std::chrono::system_clock::now().time_since_epoch().count() / 1000 << std::endl;
}
void RaftNode::StepDown() {
    std::cout << "StepDown to follower" << std::endl;
    state_ = RaftNodeState::FOLLOWER;
}


} // namespace raft
