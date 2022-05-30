#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "raft.grpc.pb.h"

class RaftConsensusServiceImpl final : public raft::RaftConsensusService::Service {
    grpc::Status AppendEntries(grpc::ServerContext *context, const raft::AppendEntriesReq *request,
                               raft::AppendEntriesResp *response) override {
        std::cout << "AppendEntriesReq: " << request->leaderid() << std::endl;
        response->set_term(2);
        return grpc::Status::OK;
    }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  RaftConsensusServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  grpc::ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}