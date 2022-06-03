#include "raft.h"

namespace raft {

/******************************************************************************
 * Raft 节点通信 RPC 客户端
 ******************************************************************************/
RaftConsensusServiceClient::RaftConsensusServiceClient() {
    std::cout << "New RaftConsensusServiceClient" << std::endl;
}

RaftConsensusServiceClient::~RaftConsensusServiceClient() {
    std::cout << "Delete RaftConsensusServiceClient" << std::endl;
}

raft::AppendEntriesResp RaftConsensusServiceClient::AppendEntries(const raft::AppendEntriesReq request, const raft::Server server) {
    std::cout << "RaftConsensusServiceClient::AppendEntries: " << request.leaderid() << ", " << server.address() << std::endl;
    std::unique_ptr<raft::RaftConsensusService::Stub> stub = NewStub(server);

    raft::AppendEntriesResp response;
    grpc::ClientContext context;
    grpc::Status status = stub->AppendEntries(&context, request, &response);
    if (status.ok()) {
        return response;
    } else {
        // TODO error handling
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        response.set_term(-1);
        return response;
    }
}

raft::RequestVoteResp RaftConsensusServiceClient::RequestVote(const raft::RequestVoteReq request, const raft::Server server) {
    std::unique_ptr<raft::RaftConsensusService::Stub> stub = NewStub(server);

    raft::RequestVoteResp response;
    grpc::ClientContext context;
    grpc::Status status = stub->RequestVote(&context, request, &response);
    if (status.ok()) {
        return response;
    } else {
        // TODO error handling
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        response.set_term(-1);
        return response;
    }
}

raft::InstallSnapshotResp RaftConsensusServiceClient::InstallSnapshot(const raft::InstallSnapshotReq request, const raft::Server server) {
    std::unique_ptr<raft::RaftConsensusService::Stub> stub = NewStub(server);

    raft::InstallSnapshotResp response;
    grpc::ClientContext context;

    grpc::Status status = stub->InstallSnapshot(&context, request, &response);
    if (status.ok()) {
        return response;
    } else {
        // TODO error handling
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        response.set_term(-1);
        return response;
    }
}

std::unique_ptr<raft::RaftConsensusService::Stub> RaftConsensusServiceClient::NewStub(const raft::Server server) {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(server.address(), grpc::InsecureChannelCredentials());
    return raft::RaftConsensusService::NewStub(channel);
}


/******************************************************************************
 * Raft 节点通信 RPC 服务实现
 ******************************************************************************/
RaftConsensusServiceImpl::RaftConsensusServiceImpl(std::function<void(const raft::AppendEntriesReq*, raft::AppendEntriesResp*)> handle_append_entries,
                                                    std::function<void(const raft::RequestVoteReq*, raft::RequestVoteResp*)> handle_request_vote,
                                                    std::function<void(const raft::InstallSnapshotReq*, raft::InstallSnapshotResp*)> handle_install_snapshot) {
    std::cout << "New RaftConsensusServiceImpl" << std::endl;
    handle_append_entries_ = handle_append_entries;
    handle_request_vote_ = handle_request_vote;
    handle_install_snapshot_ = handle_install_snapshot;
}

RaftConsensusServiceImpl::~RaftConsensusServiceImpl() {
    std::cout << "Delete RaftConsensusServiceImpl" << std::endl;
}

grpc::Status RaftConsensusServiceImpl::AppendEntries(grpc::ServerContext* context, const AppendEntriesReq* request, AppendEntriesResp* response) {
    std::cout << "RaftConsensusServiceImpl.AppendEntries: " << request->leaderid() << std::endl;
    handle_append_entries_(request, response);
    return grpc::Status::OK;
}

grpc::Status RaftConsensusServiceImpl::RequestVote(grpc::ServerContext* context, const RequestVoteReq* request, RequestVoteResp* response) {
    std::cout << "RequestVoteReq: " << request->candidateid() << std::endl;
    handle_request_vote_(request, response);
    return grpc::Status::OK;
}

grpc::Status RaftConsensusServiceImpl::InstallSnapshot(grpc::ServerContext* context, const InstallSnapshotReq* request, InstallSnapshotResp* response) {
    std::cout << "InstallSnapshotReq: " << request->leaderid() << std::endl;
    handle_install_snapshot_(request, response);
    return grpc::Status::OK;
}


/******************************************************************************
 * Raft 节点通信 RPC 服务端
 ******************************************************************************/
RaftConsensusServiceServer::RaftConsensusServiceServer(raft::Server local_server, raft::RaftConsensusServiceImpl* service_impl) : local_server_(local_server) {
    std::cout << "New RaftConsensusServiceServer" << std::endl;
    service_impl_ = service_impl;
}

RaftConsensusServiceServer::~RaftConsensusServiceServer() {
    std::cout << "Delete RaftConsensusServiceServer" << std::endl;
}

void RaftConsensusServiceServer::Start() {
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(local_server_.address(), grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(service_impl_);
    // Finally assemble the server.
    grpc_server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << local_server_.address() << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    // grpc_server_->Wait();
}

void RaftConsensusServiceServer::Stop() {
    std::cout << "Server shutdown on " << local_server_.address() << std::endl;
    grpc_server_->Shutdown();
}


} // namespace raft