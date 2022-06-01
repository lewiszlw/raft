#include "raft.h"

namespace raft {

/******************************************************************************
 * Raft 节点通信 RPC 客户端
 ******************************************************************************/
RaftConsensusServiceClient::RaftConsensusServiceClient(std::string address) {
    std::cout << "New RaftConsensusServiceClient for " << address << std::endl;
    address_ = address;
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(address_, grpc::InsecureChannelCredentials());
    stub_ = raft::RaftConsensusService::NewStub(channel);
}
RaftConsensusServiceClient::~RaftConsensusServiceClient() {
    std::cout << "Delete RaftConsensusServiceClient for " << address_ << std::endl;
}
raft::AppendEntriesResp RaftConsensusServiceClient::AppendEntries(const raft::AppendEntriesReq request) {
    raft::AppendEntriesResp reply;
    grpc::ClientContext context;
    grpc::Status status = stub_->AppendEntries(&context, request, &reply);
    if (status.ok()) {
        return reply;
    } else {
        // TODO error handling
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        reply.set_term(-1);
        return reply;
    }
}
raft::RequestVoteResp RaftConsensusServiceClient::RequestVote(const raft::RequestVoteReq request) {
    raft::RequestVoteResp reply;
    grpc::ClientContext context;
    grpc::Status status = stub_->RequestVote(&context, request, &reply);
    if (status.ok()) {
        return reply;
    } else {
        // TODO error handling
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        reply.set_term(-1);
        return reply;
    }
}
raft::InstallSnapshotResp RaftConsensusServiceClient::InstallSnapshot(const raft::InstallSnapshotReq request) {
    raft::InstallSnapshotResp reply;
    grpc::ClientContext context;
    grpc::Status status = stub_->InstallSnapshot(&context, request, &reply);
    if (status.ok()) {
        return reply;
    } else {
        // TODO error handling
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        reply.set_term(-1);
        return reply;
    }
}


/******************************************************************************
 * Raft 节点通信 RPC 服务端
 ******************************************************************************/
RaftConsensusServiceServer::RaftConsensusServiceServer(std::string address) : address_(address) {
}
RaftConsensusServiceServer::~RaftConsensusServiceServer() {
}
void RaftConsensusServiceServer::Start() {
    RaftConsensusServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << address_ << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server_->Wait();
}
void RaftConsensusServiceServer::Stop() {
    std::cout << "Server shutdown on " << address_ << std::endl;
    server_->Shutdown();
}


/******************************************************************************
 * Raft 节点通信 RPC 服务实现
 ******************************************************************************/
RaftConsensusServiceImpl::RaftConsensusServiceImpl() {
    std::cout << "RaftConsensusServiceImpl::RaftConsensusServiceImpl()" << std::endl;
}
RaftConsensusServiceImpl::~RaftConsensusServiceImpl() {
    std::cout << "RaftConsensusServiceImpl::~RaftConsensusServiceImpl()" << std::endl;
}
grpc::Status RaftConsensusServiceImpl::AppendEntries(grpc::ServerContext* context, const AppendEntriesReq* request, AppendEntriesResp* response) {
    // TODO 实现 AppendEntries RPC
    std::cout << "AppendEntriesReq: " << request->leaderid() << std::endl;
    response->set_term(2);
    return grpc::Status::OK;
}
grpc::Status RaftConsensusServiceImpl::RequestVote(grpc::ServerContext* context, const RequestVoteReq* request, RequestVoteResp* response) {
    // TODO 实现 RequestVote RPC
    std::cout << "RequestVoteReq: " << request->candidateid() << std::endl;
    response->set_term(2);
    return grpc::Status::OK;
}
grpc::Status RaftConsensusServiceImpl::InstallSnapshot(grpc::ServerContext* context, const InstallSnapshotReq* request, InstallSnapshotResp* response) {
    // TODO 实现 InstallSnapshot RPC
    std::cout << "InstallSnapshotReq: " << request->leaderid() << std::endl;
    response->set_term(2);
    return grpc::Status::OK;
}

} // namespace raft