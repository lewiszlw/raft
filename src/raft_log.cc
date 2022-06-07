#include "raft.h"

namespace raft {


RaftLog::RaftLog() {
    //std::cout << "New RaftLog" << std::endl;
    start_index_ = 1;
}
RaftLog::~RaftLog() {
    std::cout << "Delete RaftLog" << std::endl;
}

void RaftLog::Append(const AppendEntriesReq* request) {
    std::cout << "Append RaftLog " << request->entries_size() << std::endl;
    auto entries = request->entries();
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        entries_.push_back(*it);
    }
}
void RaftLog::Append(const LogEntry& new_entry) {
    std::cout << "Append RaftLog " << new_entry.DebugString() << std::endl;
    entries_.push_back(new_entry);
}

LogEntry* RaftLog::Get(uint64_t index) {
    std::cout << "Get RaftLog " << index << std::endl;
    if (index < start_index_) {
        return nullptr;
    }
    return &entries_.at(index - start_index_);
}
LogEntry* RaftLog::Get(uint64_t index, uint64_t term) {
    std::cout << "Get RaftLog " << index << " " << term << std::endl;
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->index() == index && it->term() == term) {
            return &(*it);
        }
    }
    return nullptr;
}

uint64_t RaftLog::GetLastIndex() {
    return start_index_ + entries_.size() - 1;
}

uint64_t RaftLog::GetLastTerm() {
    // TODO
    if (entries_.size() == 0) {
        return 0;
    }
    return entries_.back().term();
}


} // namespace raft