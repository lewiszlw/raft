#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "raft.grpc.pb.h"

class RaftConsensusServiceClient {
 public:
  RaftConsensusServiceClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(raft::RaftConsensusService::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  raft::AppendEntriesResp AppendEntries(const raft::AppendEntriesReq request) {
    // Container for the data we expect from the server.
    raft::AppendEntriesResp reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->AppendEntries(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      reply.set_term(-1);
      return reply;
    }
  }

 private:
  std::unique_ptr<raft::RaftConsensusService::Stub> stub_;
};

int main(int argc, char** argv) {
  // 实例化Client
  RaftConsensusServiceClient raft_consensus_service_client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

  raft::AppendEntriesReq request;
  request.set_leaderid(1);
  raft::AppendEntriesResp reply = raft_consensus_service_client.AppendEntries(request);
  std::cout << "RaftConsensusServiceClient received: " << reply.term() << std::endl;

  return 0;
}