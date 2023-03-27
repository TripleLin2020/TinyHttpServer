#ifndef TINY_HTTP_SERVER_IO_CONTEXT_POOL_H
#define TINY_HTTP_SERVER_IO_CONTEXT_POOL_H

#include <boost/asio.hpp>
#include <memory>
#include <vector>

#define asio boost::asio

class IoContextPool {
public:
    explicit IoContextPool(std::size_t poolSize) : _nextIoContext(0) {
        if (poolSize <= 0) poolSize = 1;

        for (std::size_t i = 0; i < poolSize; ++i) {
            IoContextPtr ioContext = std::make_shared<asio::io_context>();
            WorkPtr work = std::make_shared<asio::io_context::work>(*ioContext);
            _ioContexts.push_back(ioContext);
            _works.push_back(work);
        }
    }

    void run() {
        std::vector<std::shared_ptr<std::thread>> threads;

        for (auto& ctx : _ioContexts) {
            threads.emplace_back(std::make_shared<std::thread>([](auto p) { p->run(); }, ctx));
        }

        for (auto& t : threads) {
            t->join();
        }
    }

    asio::io_context& getIoContext() {
        asio::io_context& ioContext = *_ioContexts[_nextIoContext];
        ++_nextIoContext;
        if (_nextIoContext == _ioContexts.size()) _nextIoContext = 0;
        return ioContext;
    }

private:
    using IoContextPtr = std::shared_ptr<asio::io_context>;
    using WorkPtr = std::shared_ptr<asio::io_context::work>;

    std::size_t _nextIoContext;
    std::vector<IoContextPtr> _ioContexts;
    std::vector<WorkPtr> _works;
};

#undef asio

#endif  // TINY_HTTP_SERVER_IO_CONTEXT_POOL_H
