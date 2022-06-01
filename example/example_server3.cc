#include <iostream>
#include "raft.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    raft::Server local_server;
    local_server.set_address("localhost:50053");
    local_server.set_server_id(3);

    std::vector<raft::Server> peer_servers;
    raft::Server peer_server1;
    peer_server1.set_address("localhost:50051");
    peer_server1.set_server_id(1);
    raft::Server peer_server2;
    peer_server2.set_address("localhost:50052");
    peer_server2.set_server_id(2);
    peer_servers.push_back(peer_server1);
    peer_servers.push_back(peer_server2);

    raft::RaftNode raft_node(local_server, peer_servers);
    raft_node.Start();

    return 0;
}