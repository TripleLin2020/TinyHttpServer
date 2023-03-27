#ifndef TINY_HTTP_SERVER_IO_CONTEXT_POOL_H
#define TINY_HTTP_SERVER_IO_CONTEXT_POOL_H

#include <memory>
#include <vector>
#include <boost/asio.hpp>

#define asio boost::asio

class IoContextPool
{
public:
    explicit IoContextPool(std::size_t poolSize) : nextIoContext_(0) {
        if (poolSize <= 0) poolSize = 1;

        for (std::size_t i = 0; i < poolSize; ++i) {
            IoContextPtr ioContext(new asio::io_context);
            WorkPtr work(new asio::io_context::work(*ioContext));
            ioContexts_.push_back(ioContext);
            works_.push_back(work);
        }
    }

    void run() {
        std::vector<std::shared_ptr<std::thread>> threads;
        for (auto& ctx: ioContexts_) {
            threads.emplace_back(std::make_shared<std::thread>([](IoContextPtr p) { p->run(); }, ctx));
        }

        for (auto& t: threads) {
            t->join();
        }
    }

    asio::io_context& getIoContext() {
        asio::io_context& ioContext = *ioContexts_[nextIoContext_];
        ++nextIoContext_;
        if (nextIoContext_ == ioContexts_.size()) nextIoContext_ = 0;
        return ioContext;
    }

private:
    using IoContextPtr = std::shared_ptr<asio::io_context>;
    using WorkPtr = std::shared_ptr<asio::io_context::work>;

    std::size_t nextIoContext_;
    std::vector<IoContextPtr> ioContexts_;
    std::vector<WorkPtr> works_;
};

#undef asio

#endif //TINY_HTTP_SERVER_IO_CONTEXT_POOL_H
