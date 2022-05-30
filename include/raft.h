//
// Created by Linwei Zhang on 2022/4/28.
//

#ifndef RAFT_RAFT_H
#define RAFT_RAFT_H

class RaftService {
    public:
    private:
        void appendEntires();
        void requestVote();
        void installSnapshot();

};

#endif //RAFT_RAFT_H
