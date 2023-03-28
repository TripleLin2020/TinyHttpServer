#ifndef TINY_HTTP_SERVER_CONDITION_H
#define TINY_HTTP_SERVER_CONDITION_H

#include <condition_variable>
#include <mutex>

class Condition {
public:
    void release() {
        std::lock_guard lock(_mutex);
        ++_count;
        _condition.notify_one();
    }

    void acquire() {
        std::unique_lock lock(_mutex);
        _condition.wait(lock, [this] { return _count > 0; });
        --_count;
    }

private:
    std::mutex _mutex;
    std::condition_variable _condition;
    std::size_t _count = 0;
};

#endif  // TINY_HTTP_SERVER_CONDITION_H
