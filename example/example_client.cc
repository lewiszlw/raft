#include <iostream>
#include "raft.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    std::string server_address("localhost:50051");
    raft::Server server;
    server.set_address(server_address);

    raft::RaftConsensusServiceClient raft_consensus_service_client;

    raft::AppendEntriesReq append_entries_req;
    append_entries_req.set_leaderid(1);

    raft::AppendEntriesResp append_entries_resp = raft_consensus_service_client.AppendEntries(append_entries_req, server);
    std::cout << "RaftConsensusServiceClient received: " << append_entries_resp.term() << std::endl;

    return 0;
}