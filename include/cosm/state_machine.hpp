#pragma once

#include <boost/async.hpp>
#include <boost/asio/detached.hpp>

namespace cosm {
namespace async = boost::async;
namespace asio = boost::asio;

template <typename SM, typename Event, std::size_t QueueCapacity = 0u>
struct state_machine {
    SM* sm_;
    async::channel<Event> event_queue_{QueueCapacity};

    struct transition;
    using state = async::promise<transition>;
    using state_fn_ptr = state (SM::*)();
    struct transition { state_fn_ptr next; };

    auto operator co_await() {
        return event_queue_.read();
    }

    async::promise<void> process_event(Event ev) {
        co_await event_queue_.write(std::move(ev));
    }

    bool try_process_event(Event ev) {
        return event_queue_.try_write(std::move(ev));
    }

    async::task<void> run(state_fn_ptr initial) {
        transition next{initial};
        for (;;) {
            next = co_await std::invoke(next.next, sm_);
            if (next.next == nullptr) {
                break;
            }
        }
        co_return;
    }
};
} // cosm namespace
