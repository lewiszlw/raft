//
// Created by Linwei Zhang on 2022/4/29.
//

#include <iostream>
#include "raft.pb.h"

int main() {
    raft::LogEntry logEntry;
    logEntry.set_index(10);
    logEntry.set_term(10);
    std::cout << "index: " << logEntry.index() << ", term: " << logEntry.term() << std::endl;

    raft::AppendEntriesReq appendEntriesReq;
    appendEntriesReq.set_term(20);
    raft::LogEntry *newLogEntry = appendEntriesReq.add_entries();
    newLogEntry->set_term(30);
    newLogEntry->set_index(30);
    std::cout << "appendEntriesReq: " << appendEntriesReq.entries().Get(0).term() << std::endl;

    raft::AppendEntriesResp appendEntriesResp;
    appendEntriesResp.set_success(false);
    std::cout << "appendEntriesResp: " << (appendEntriesResp.success() ? "true" : "false") << std::endl;
    
    return 0;
}
