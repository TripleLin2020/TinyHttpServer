#ifndef TINY_HTTP_SERVER_SYNC_AWAIT_H
#define TINY_HTTP_SERVER_SYNC_AWAIT_H

#include "Condition.h"
#include "Try.h"

template <typename LazyType>
inline auto syncAwait(LazyType&& lazy) {
    auto executor = lazy.getExecutor();
    if (executor)
        logicAssert(!executor->currentThreadInExecutor(),
                    "Do not asyncAwait in the same executor with Lazy.");
    Condition cond;
    using ValueType = typename std::decay_t<LazyType>::ValueType;

    Try<ValueType> value;
    std::move(std::forward<LazyType>(lazy)).start([&cond, &value](Try<ValueType> result) {
        value = std::move(result);
        cond.release();
    });
    cond.acquire();
    return std::move(value).value();
}

#endif  // TINY_HTTP_SERVER_SYNC_AWAIT_H
