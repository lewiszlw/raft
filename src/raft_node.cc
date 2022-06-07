#include "raft.h"
#include <unistd.h>


namespace raft {

RaftNode::RaftNode(Server local_server, std::vector<Server> peer_servers) {
    // 初始化节点状态为Follower
    state_ = RaftNodeState::FOLLOWER;
    voted_for_ = 0;

    // 任期从0开始单调递增
    current_term_ = 0;
    // TODO 临时测试
    // current_term_ = local_server.server_id();
    // 已提交日志索引，从0开始单调递增
    commit_index_ = 0;
    // 已应用到状态机日志索引，从0开始单调递增
    last_applied_ = 0;

    // TODO 加载snapshot

    // 初始化rpc客户端和服务端
    raft_consensus_service_client_ = new RaftConsensusServiceClient();
    raft_consensus_service_impl_ = new RaftConsensusServiceImpl(std::bind(&RaftNode::HandleAppendEntries, this, std::placeholders::_1, std::placeholders::_2),
                                                                std::bind(&RaftNode::HandleRequestVote, this, std::placeholders::_1, std::placeholders::_2),
                                                                std::bind(&RaftNode::HandleInstallSnapshot, this, std::placeholders::_1, std::placeholders::_2));
    raft_consensus_service_server_ = new RaftConsensusServiceServer(&local_server_, raft_consensus_service_impl_);

    // 初始化计时器
    election_timer_ = new RaftTimer();
    heartbeat_timer_ = new RaftTimer();
    snapshot_timer_ = new RaftTimer();

    local_server_ = local_server;
    // 初始化其他服务器
    peer_manager_.AddPeers(peer_servers, log_.GetLastIndex() + 1);
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

void RaftNode::Replicate(std::string data) {
    if (state_ != RaftNodeState::LEADER) {
        // TODO 报错
        return;
    }

    LogEntry log_entry;
    log_entry.set_term(current_term_);
    log_entry.set_index(log_.GetLastIndex() + 1);
    log_entry.set_type(EntryType::DATA);
    log_entry.set_data(data);

    // 将 entry 存入 log
    log_.Append(log_entry);

    // 将 entry 发送给所有的其他节点
    AppendEntries();
}

void RaftNode::AppendEntries() {
    // std::cout << "RaftNode::AppendEntries" << std::endl;
    // TODO 但某个服务器prev日志不匹配时，需要发送更前面的日志
    std::vector<RaftPeer> peers = peer_manager_.GetPeers();
    for (size_t i = 0; i < peers.size(); i++) {
        raft::AppendEntriesReq request;
        request.set_leaderid(local_server_.server_id());
        request.set_term(current_term_);
        request.set_leadercommit(commit_index_);
        LogEntry* prev_log = log_.Get(peers[i].next_index_ - 1);
        if (prev_log == nullptr) {
            request.set_prevlogindex(-1);
            request.set_prevlogterm(-1);
        } else {
            request.set_prevlogindex(prev_log->index());
            request.set_prevlogterm(prev_log->term());
            LogEntry* log_entry = request.add_entries();
            log_entry = prev_log;
        }
        AppendEntriesResp response = raft_consensus_service_client_->AppendEntries(request, peers[i].server_);
        if (response.success()) {
            // 更新 match_index 和 next_index
            peers[i].match_index_ = log_.GetLastIndex();
            peers[i].next_index_ = log_.GetLastIndex() + 1;
        }
    }
}

void RaftNode::RequestVote() {
    // std::cout << "RaftNode::RequestVote" << std::endl;
    uint32_t vote_granted_count = 0;
    // TODO 并行地向所有的其他节点请求投票
    std::vector<RaftPeer> peers = peer_manager_.GetPeers();
    for (size_t i = 0; i < peers.size(); i++) {
        RequestVoteReq request;
        request.set_candidateid(local_server_.server_id());
        request.set_term(current_term_);
        request.set_lastlogindex(log_.GetLastIndex());
        request.set_lastlogterm(log_.GetLastTerm());
        RequestVoteResp response = raft_consensus_service_client_->RequestVote(request, peers[i].server_);
        if (response.term() > current_term_) {
            state_ = RaftNodeState::FOLLOWER;
            return;
        }
        if (response.votegranted()) {
            vote_granted_count++;
            peers[i].vote_granted_ = true;
        }
        std::cout << "vote_granted_count: " << vote_granted_count << std::endl;
        // 获得多数投票（包含自身投票）后成为leader
        if (vote_granted_count + 1 > (peers.size() + 1) / 2) {
            std::cout << "Become leader" << std::endl;
            state_ = RaftNodeState::LEADER;
            // 成为leader后立刻发送心跳
            AppendEntries();
            // 关闭选举计时器
            election_timer_->Deschedule();
        }

    }
}
void RaftNode::InstallSnapshot() {
    std::cout << "RaftNode::InstallSnapshot" << std::endl;
}

void RaftNode::HandleAppendEntries(const raft::AppendEntriesReq* request, raft::AppendEntriesResp* response) {
    if (state_ == RaftNodeState::FOLLOWER) {
        // 如果是follower
        if (request->term() < current_term_) {
            // 如果leader的term小于自己的term，则忽略
            response->set_term(current_term_);
            response->set_success(false);
            return;
        }
        LogEntry* prev_log = log_.Get(request->prevlogindex(), request->prevlogterm());
        if (prev_log == nullptr) {
            // 如果prev_log不存在，则忽略
            response->set_term(current_term_);
            response->set_success(false);
            return;
        }
        log_.Append(request);
        // 重置选举计时器
        election_timer_->Reset(10000);

    } else if (state_ == RaftNodeState::CANDIDATE) {
        // 如果是candidate，leader任期不小于自己的任期，则承认leader地位，转为follower
        if (request->term() >= current_term_) {
            state_ = RaftNodeState::FOLLOWER;
            // TODO 终止请求投票过程
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
    if (state_ == RaftNodeState::FOLLOWER) {
        // 本身为follower
        // 如果candidate任期小于自己的任期，则拒绝投票
        if (request->term() < current_term_) {
            response->set_term(current_term_);
            response->set_votegranted(false);
            return;
        }

        // 如果未投票或已投票给candidate，则本次投票给candidate
        if (voted_for_ == 0 || voted_for_ == request->candidateid()) {
            std::cout << "Vote for " << request->candidateid() << std::endl;
            response->set_votegranted(true);
            response->set_term(current_term_);
            voted_for_ = request->candidateid();
            // 重置选举计时器
            election_timer_->Reset(10000);
            return;
        }

    } else if (state_ == RaftNodeState::CANDIDATE) {
        // 本身为candidate，则拒绝投票
        std::cout << "Refuse RequestVote from candidate because I'm candidate too " << request->DebugString() << std::endl;
        response->set_term(current_term_);
        response->set_votegranted(false);

    } else if (state_ == RaftNodeState::LEADER) {
        // 本身为leader，TODO

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
        // 给自己投票
        voted_for_ = local_server_.server_id();
        // 重置选举计时器
        //std::cout << "Reset election timer" << std::endl;
        election_timer_->Reset(10000);   // 随机时间
        // 发送投票请求
        RequestVote();

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
