#ifndef TINY_HTTP_SERVER_EXECUTOR_H
#define TINY_HTTP_SERVER_EXECUTOR_H

#include <chrono>
#include <functional>
#include <string>
#include <thread>

struct ScheduleOptions {
    /// Whether or not this schedule should be prompted.
    bool prompt = true;
    ScheduleOptions() = default;
};

struct CurrentExecutor {};

class Executor {
public:
    using Context = void*;
    static constexpr Context* NULLCTX = nullptr;

    using Duration = std::chrono::duration<int64_t, std::micro>;

    using Func = std::function<void()>;

    virtual bool schedule(Func func) = 0;

    virtual bool currentThreadInExecutor() const { throw std::logic_error("Not implemented"); }

    virtual Context checkout() { return NULLCTX; }
    virtual bool checkin(Func func, [[maybe_unused]] Context ctx,
                         [[maybe_unused]] ScheduleOptions opts) {
        return schedule(std::move(func));
    }

    virtual bool checkin(Func func, Context ctx) {
        static ScheduleOptions opts;
        return checkin(std::move(func), ctx, opts);
    }

private:
    virtual void schedule(Func func, Duration dur) {
        std::thread([this, func = std::move(func), dur]() {
            std::this_thread::sleep_for(dur);
            schedule(std::move(func));
        }).detach();
    }

private:
    std::string name_;
};

#endif  // TINY_HTTP_SERVER_EXECUTOR_H
