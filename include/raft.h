//
// Created by Linwei Zhang on 2022/4/28.
//

#ifndef RAFT_RAFT_H
#define RAFT_RAFT_H

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/grpcpp.h>
#include "raft.grpc.pb.h"

namespace raft {


/******************************************************************************************
 *  RaftConsensusService 节点通信 RPC
 * ****************************************************************************************/
// Raft 节点通信 RPC 客户端
class RaftConsensusServiceClient {
    public:
        RaftConsensusServiceClient(std::string address);
        ~RaftConsensusServiceClient();
        raft::AppendEntriesResp AppendEntries(const raft::AppendEntriesReq request);
        raft::RequestVoteResp RequestVote(const raft::RequestVoteReq request);
        raft::InstallSnapshotResp InstallSnapshot(const raft::InstallSnapshotReq request);
    private:
        std::string address_;
        std::unique_ptr<raft::RaftConsensusService::Stub> stub_;
};
// Raft 节点通信 RPC 服务端
class RaftConsensusServiceServer {
    public:
        RaftConsensusServiceServer(std::string address);
        ~RaftConsensusServiceServer();
        void Start();
        void Stop();
    private:
        std::string address_;
        std::unique_ptr<grpc::Server> server_;
};
// Raft 节点通信 RPC 服务实现
class RaftConsensusServiceImpl final : public raft::RaftConsensusService::Service {
    public:
        RaftConsensusServiceImpl();
        ~RaftConsensusServiceImpl();
        grpc::Status AppendEntries(grpc::ServerContext* context, const raft::AppendEntriesReq* request, raft::AppendEntriesResp* response) override;
        grpc::Status RequestVote(grpc::ServerContext* context, const raft::RequestVoteReq* request, raft::RequestVoteResp* response) override;
        grpc::Status InstallSnapshot(grpc::ServerContext* context, const raft::InstallSnapshotReq* request, raft::InstallSnapshotResp* response) override;
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


// Raft 节点
class RaftNode {
    public:
        RaftNode();
        ~RaftNode();
        void Start();
        void Stop();
        void AppendEntries();
        void RequestVote();
        void InstallSnapshot();
        void StepDown();                                                                          // 回退为Follower
    private:
        const uint64_t server_id_;                                                                // 节点ID
        std::string address_;                                                                     // 本节点的网络地址
        RaftStateMachine raft_state_machine_;
        RaftConsensusServiceClient raft_consensus_service_client_;
        RaftConsensusServiceServer raft_consensus_service_server_;
};



} // namespace raft

#endif //RAFT_RAFT_H
