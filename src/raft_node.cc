#include "raft.h"
#include <unistd.h>


namespace raft {

RaftNode::RaftNode(raft::Server local_server, std::vector<raft::Server> peer_servers) {
    server_ = local_server;
    for (size_t i = 0; i < peer_servers.size(); i++) {
        RaftPeer peer(peer_servers[i]);
        peers_.push_back(peer);
    }
    static RaftConsensusServiceClient raft_consensus_service_client;
    raft_consensus_service_client_ = &raft_consensus_service_client;
    static RaftConsensusServiceServer raft_consensus_service_server(local_server);
    raft_consensus_service_server_ = &raft_consensus_service_server;

}
RaftNode::~RaftNode() {
}
void RaftNode::Start() {
    // 启动 Raft 节点
    // 启动 Raft 节点的 RPC 服务
    raft_consensus_service_server_->Start();

    // TODO 测试
    sleep(15);
    AppendEntries();

    raft_consensus_service_server_->grpc_server_->Wait();

    // 启动 Raft 节点的定时器
    // 启动 Raft 节点的状态机
}
void RaftNode::AppendEntries() {
    std::cout << "RaftNode::AppendEntries" << std::endl;
    // TODO
    for (size_t i = 0; i < peers_.size(); i++) {
        raft::AppendEntriesReq request;
        request.set_leaderid(server_.server_id());
        raft_consensus_service_client_->AppendEntries(request, peers_[i].server_);
    }
}


} // namespace raft
