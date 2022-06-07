//
// Created by Linwei Zhang on 2022/4/28.
//

#ifndef RAFT_RAFT_H
#define RAFT_RAFT_H

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/grpcpp.h>
#include "raft.grpc.pb.h"
#include <thread>
#include <chrono>
#include <deque>

namespace raft {

// Raft 节点状态枚举类
enum class RaftNodeState { UKNOWN, FOLLOWER, CANDIDATE, LEADER };


/******************************************************************************************
 *  Raft 计时器
 * ****************************************************************************************/
class RaftTimer {
    public:
        RaftTimer();
        ~RaftTimer();
        void Schedule(uint32_t timeout_ms, std::function<void()> callback);
        void Deschedule();
        void Reset(uint32_t timeout_ms);
    private:
        std::function<void()> callback_;
        std::thread thread_;
        bool is_running_;
        uint32_t timeout_ms_;
        std::chrono::system_clock::time_point next_timeout_at_;
};

// Raft 日志
class RaftLog {
    public:
        RaftLog();
        ~RaftLog();
        void Append(const AppendEntriesReq* request);
        void Append(const LogEntry& new_entry);
        LogEntry* Get(uint64_t index);
        LogEntry* Get(uint64_t index, uint64_t term);
        uint64_t GetLastIndex();
        uint64_t GetLastTerm();
        //LogEntry& GetLastEntry();
        //uint64_t GetCommittedIndex();
        //void SetCommittedIndex(uint64_t index);
    private:
        std::deque<LogEntry> entries_;
        uint64_t start_index_;
        //uint64_t committed_index_;
};


/******************************************************************************************
 *  Raft 节点通信 RPC
 * ****************************************************************************************/
// Raft 节点通信 RPC 客户端
class RaftConsensusServiceClient {
    public:
        RaftConsensusServiceClient();
        ~RaftConsensusServiceClient();
        raft::AppendEntriesResp AppendEntries(const AppendEntriesReq request, const Server server);
        raft::RequestVoteResp RequestVote(const RequestVoteReq request, const Server server);
        raft::InstallSnapshotResp InstallSnapshot(const InstallSnapshotReq request, const Server server);
    private:
        std::unique_ptr<raft::RaftConsensusService::Stub> NewStub(const Server server);
};
// Raft 节点通信 RPC 服务实现
class RaftConsensusServiceImpl final : public raft::RaftConsensusService::Service {
    public:
        RaftConsensusServiceImpl(std::function<void(const AppendEntriesReq*, AppendEntriesResp*)> handle_append_entries,
                                std::function<void(const RequestVoteReq*, RequestVoteResp*)> handle_request_vote,
                                std::function<void(const InstallSnapshotReq*, InstallSnapshotResp*)> handle_install_snapshot);
        ~RaftConsensusServiceImpl();
        grpc::Status AppendEntries(grpc::ServerContext* context, const raft::AppendEntriesReq* request, raft::AppendEntriesResp* response) override;
        grpc::Status RequestVote(grpc::ServerContext* context, const raft::RequestVoteReq* request, raft::RequestVoteResp* response) override;
        grpc::Status InstallSnapshot(grpc::ServerContext* context, const raft::InstallSnapshotReq* request, raft::InstallSnapshotResp* response) override;
    private:
        std::function<void(const AppendEntriesReq*, AppendEntriesResp*)> handle_append_entries_;
        std::function<void(const RequestVoteReq*, RequestVoteResp*)> handle_request_vote_;
        std::function<void(const InstallSnapshotReq*, InstallSnapshotResp*)> handle_install_snapshot_;
};
// Raft 节点通信 RPC 服务端
class RaftConsensusServiceServer {
    public:
        RaftConsensusServiceServer(Server* local_server, RaftConsensusServiceImpl* service_impl);
        ~RaftConsensusServiceServer();
        void Start();
        void Stop();
        Server* local_server_;
        std::unique_ptr<grpc::Server> grpc_server_;
    private:
        raft::RaftConsensusServiceImpl* service_impl_;
};


/******************************************************************************************
 *  Raft 状态机
 * ****************************************************************************************/
// Raft 状态机
class RaftStateMachine {
    public:
        RaftStateMachine();
        ~RaftStateMachine();
        void Apply(LogEntry log_entry);
        void WriteSnapshot();
        void ReadSnapshot();
};


/******************************************************************************************
 *  Raft 节点
 * ****************************************************************************************/
// Raft 其他服务器节点
class RaftPeer {
    public:
        RaftPeer(Server server);
        ~RaftPeer();
        Server server_;                                                                           // 服务器信息
        uint64_t next_index_;                                                                     // 发送到该服务器的下一个日志条目的索引
        uint64_t match_index_;                                                                    // 已知的已经复制到该服务器的最高日志条目的索引
        bool vote_granted_;                                                                       // 是否投票给本节点
        bool is_leader_;                                                                          // 是否为 leader
    private:
};
class RaftPeerManager {
    public:
        RaftPeerManager();
        ~RaftPeerManager();
        void AddPeers(std::vector<Server> servers, uint64_t next_index);
        RaftPeer* GetLeader();
        std::vector<RaftPeer>& GetPeers();
    private:
        std::vector<RaftPeer> peers_;
};
// Raft 本实例节点
class RaftNode {
    public:
        RaftNode(raft::Server local_server, std::vector<raft::Server> peer_servers);
        ~RaftNode();
        void Start();
        void Stop();

        void Replicate(std::string data);                                                         // 客户端请求复制命令内容

        void AppendEntries();                                                                     // leader附加日志
        void RequestVote();                                                                       // candidate请求投票
        void InstallSnapshot();                                                                   // leader安装快照

        void HandleAppendEntries(const AppendEntriesReq* request, AppendEntriesResp* response);
        void HandleRequestVote(const RequestVoteReq* request, RequestVoteResp* response);
        void HandleInstallSnapshot(const InstallSnapshotReq* request, InstallSnapshotResp* response);

        void StepDown();                                                                          // 回退为Follower

        void HandleElectionTimeout();                                                             // 处理选举超时
        void HandleHeartbeatTimeout();                                                            // 处理心跳超时
        void HandleSnapshotTimeout();                                                             // 处理快照超时

        Server local_server_;                                                                     // 服务器信息
        uint64_t current_term_;                                                                   // 服务器已知最新的任期
    private:
        RaftNodeState state_;                                                                     // 本节点的状态
        RaftTimer* election_timer_;                                                               // 选举计时器
        RaftTimer* heartbeat_timer_;                                                              // 心跳计时器
        RaftTimer* snapshot_timer_;                                                               // 快照计时器
        uint64_t voted_for_;                                                                      // 当前选举的候选人ID
        uint64_t commit_index_;                                                                   // 已知已提交的最高的日志条目的索引
        uint64_t last_applied_;                                                                   // 已经被应用到状态机的最高的日志条目的索引
        RaftPeerManager peer_manager_;                                                            // 节点列表
        RaftLog log_;                                                                             // 日志
        // RaftStateMachine raft_state_machine_;
        RaftConsensusServiceClient* raft_consensus_service_client_;
        RaftConsensusServiceImpl* raft_consensus_service_impl_;
        RaftConsensusServiceServer* raft_consensus_service_server_;
};



} // namespace raft

#endif //RAFT_RAFT_H
