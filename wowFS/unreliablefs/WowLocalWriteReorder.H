#ifndef __WOW_LOCAL_QUEUE__
#define __WOW_LOCAL_QUEUE__

#include <string>
#include <signal.h>
#include <algorithm>
#include <vector>
#include <random>
#include "unreliablefs_ops.h"

const std::string WOW_KILL_PHRASE = "__WOW_KILL__";

// Shuffle given vector
template <typename Object>
void FisherYatesShuffle(std::vector<Object> &vec)
{
    // Use a random number generator to shuffle the elements
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int i = vec.size() - 1; i > 0; --i)
    {
        // Generate a random index between 0 and i (inclusive)
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(gen);

        // Swap the elements at indices i and j
        std::swap(vec[i], vec[j]);
    }
}

// Record arguments to write operation
struct WriteOp 
{
    std::string path;
    std::string buf;
    size_t size;
    off_t offset;
    struct fuse_file_info fi;
    WriteOp(std::string path, std::string buf, size_t size, off_t offset, struct fuse_file_info *fi)
    {
        this->path = std::string(path);
        this->buf = std::string(buf);
        this->size = size;
        this->offset = offset;
        this->fi = *fi;
    }
};

class WowLocalWriteReorder
{
private:
    // Queue of pending writes
    std::vector<WriteOp> WRITE_QUEUE;

public:
    static WowLocalWriteReorder& Instance(){
        static WowLocalWriteReorder obj;
        return obj;
    }

    void RecordWrite(const char * path, const char*buf, size_t size, off_t offset, struct fuse_file_info *fi)
    {
        WRITE_QUEUE.push_back(WriteOp(path, buf, size, offset, fi));
    }

    void Shuffle()
    {
        FisherYatesShuffle(this->WRITE_QUEUE);
    }

    std::vector<WriteOp> * GetQueue()
    {
        return &WRITE_QUEUE;
    }
};

#endif