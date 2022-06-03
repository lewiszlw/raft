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

namespace raft {

// Raft 节点状态枚举类
enum class RaftNodeState { UKNOWN, FOLLOWER, CANDIDATE, LEADER };

// Raft 定时器
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

// Raft 节点
class RaftPeer {
    public:
        RaftPeer(raft::Server server);
        ~RaftPeer();
        raft::Server server_;                                                                     // 服务器信息
        uint64_t next_index_;                                                                     // 下一个日志索引
        uint64_t match_index_;                                                                    // 已经匹配的日志索引
        bool vote_granted_;                                                                       // 是否投票给本节点
    private:
};


/******************************************************************************************
 *  RaftConsensusService 节点通信 RPC
 * ****************************************************************************************/
// Raft 节点通信 RPC 客户端
class RaftConsensusServiceClient {
    public:
        RaftConsensusServiceClient();
        ~RaftConsensusServiceClient();
        raft::AppendEntriesResp AppendEntries(const raft::AppendEntriesReq request, const raft::Server server);
        raft::RequestVoteResp RequestVote(const raft::RequestVoteReq request, const raft::Server server);
        raft::InstallSnapshotResp InstallSnapshot(const raft::InstallSnapshotReq request, const raft::Server server);
    private:
        std::unique_ptr<raft::RaftConsensusService::Stub> NewStub(const raft::Server server);
};
// Raft 节点通信 RPC 服务实现
class RaftConsensusServiceImpl final : public raft::RaftConsensusService::Service {
    public:
        RaftConsensusServiceImpl(std::function<void(const raft::AppendEntriesReq*, raft::AppendEntriesResp*)> handle_append_entries,
                                std::function<void(const raft::RequestVoteReq*, raft::RequestVoteResp*)> handle_request_vote,
                                std::function<void(const raft::InstallSnapshotReq*, raft::InstallSnapshotResp*)> handle_install_snapshot);
        ~RaftConsensusServiceImpl();
        grpc::Status AppendEntries(grpc::ServerContext* context, const raft::AppendEntriesReq* request, raft::AppendEntriesResp* response) override;
        grpc::Status RequestVote(grpc::ServerContext* context, const raft::RequestVoteReq* request, raft::RequestVoteResp* response) override;
        grpc::Status InstallSnapshot(grpc::ServerContext* context, const raft::InstallSnapshotReq* request, raft::InstallSnapshotResp* response) override;
    private:
        std::function<void(const raft::AppendEntriesReq*, raft::AppendEntriesResp*)> handle_append_entries_;
        std::function<void(const raft::RequestVoteReq*, raft::RequestVoteResp*)> handle_request_vote_;
        std::function<void(const raft::InstallSnapshotReq*, raft::InstallSnapshotResp*)> handle_install_snapshot_;
};
// Raft 节点通信 RPC 服务端
class RaftConsensusServiceServer {
    public:
        RaftConsensusServiceServer(raft::Server local_server, raft::RaftConsensusServiceImpl* service_impl);
        ~RaftConsensusServiceServer();
        void Start();
        void Stop();
        raft::Server local_server_;
        std::unique_ptr<grpc::Server> grpc_server_;
    private:
        raft::RaftConsensusServiceImpl* service_impl_;
};


// Raft 状态机
class RaftStateMachine {
    public:
        RaftStateMachine();
        ~RaftStateMachine();
        void Apply();
        void WriteSnapshot();
        void ReadSnapshot();
};


// Raft 本实例节点
class RaftNode {
    public:
        RaftNode(raft::Server local_server, std::vector<raft::Server> peer_servers);
        ~RaftNode();
        void Start();
        void Stop();

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

        raft::Server local_server_;                                                               // 服务器信息
        uint64_t current_term_;                                                                   // 当前任期
    private:
        RaftNodeState state_;                                                                     // 本节点的状态
        RaftTimer* election_timer_;                                                                // 选举定时器
        RaftTimer* heartbeat_timer_;                                                               // 心跳定时器
        RaftTimer* snapshot_timer_;                                                                // 快照定时器
        uint64_t voted_for_;                                                                      // 当前选举的候选人ID
        uint64_t commit_index_;                                                                   // 当前已提交的日志索引
        uint64_t last_applied_index_;                                                             // 当前已应用的日志索引
        std::vector<RaftPeer> peers_;                                                             // 节点列表
        // RaftStateMachine raft_state_machine_;
        RaftConsensusServiceClient* raft_consensus_service_client_;
        RaftConsensusServiceImpl* raft_consensus_service_impl_;
        RaftConsensusServiceServer* raft_consensus_service_server_;
};



} // namespace raft

#endif //RAFT_RAFT_H
