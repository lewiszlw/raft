#include <iostream>
#include "raft.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    std::string server_address("0.0.0.0:50051");
    raft::RaftConsensusServiceServer raft_consensus_service_server(server_address);
    raft_consensus_service_server.Start();

    return 0;
}