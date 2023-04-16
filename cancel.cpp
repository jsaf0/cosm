
#include <iostream>

#include <boost/asio.hpp>
#include <boost/async.hpp>
#include <boost/sam.hpp>
#include <boost/async/main.hpp>

#include "signal.hpp"

using namespace std;
using namespace boost;

struct Service {
    Service(std::string name)
        : name_{std::move(name)}, started_{false} {}

    async::promise<void> Start()
    {
        cout << name_ << " starting\n";
        try {
            asio::steady_timer t{co_await async::this_coro::executor, 100ms};
            co_await t.async_wait(async::use_op);
            started_ = true;
            cout << name_ << " started\n";
        } catch (...) {
            cout << "start got cancelled\n";
            std::rethrow_exception(std::current_exception());
        }
    }

    async::promise<void> Stop()
    {
        if (!started_) {
            cout << name_ << " not started\n";
            co_return;
        }

        cout << name_ << " stopping\n";
        asio::steady_timer t{co_await async::this_coro::executor, 100ms};
        co_await t.async_wait(async::use_op);
        cout << name_ << " stopped\n";
    }

private:
    std::string name_;
    bool started_;
};

struct ServiceManager {
    void Add(Service& srv)
    {
        services_.push_back(&srv);
    }

    async::promise<void> Start()
    {
        for (auto srv : services_) {
            co_await srv->Start();
        }
    }

    async::promise<void> Stop()
    {
        for (auto srv : services_) {
            co_await srv->Stop();
        }
        //if (stopped_) {
            stopped_();
        // }
    }

    // std::function<void()> stopped_;
    sigslot::signal<> stopped_;

private:
    std::vector<Service*> services_;
};

// async::promise<int> slow_task(int i)
// {
//     auto ex = co_await async::this_coro::executor;
//     asio::steady_timer t{ex, 100ms};
//     co_await t.async_wait(async::use_op);
//     co_return i;
// }

// async::promise<void> run_all_slow_tasks(/*async::channel<void>& ch*/)
// {
//     try {
//         for (int i = 0; i < 10; i++) {
//             cout << "Got " << co_await slow_task(i) << "\n";
//         }
//         // co_await ch.write();
//     } catch (const system::system_error& e) {
//         if (e.code() == system::errc::operation_canceled) {
//             cout << "Cancelled\n";
//         }
//     }
// }

// void spawn_foo()
// {
//     +run_all_slow_tasks();
// }

async::promise<void> wait_for(sigslot::signal<>& sig)
{
    sam::condition_variable cv{co_await async::this_coro::executor};
    auto conn = sig.connect([&cv]() { cv.notify_one(); });
    co_await cv.async_wait(async::use_op);
    conn.disconnect();
}

boost::async::main co_main(int, char**)
{
    cout << "Hello\n";
    
    // auto t = run_all_slow_tasks(/*ch*/);
    // asio::steady_timer timer{co_await async::this_coro::executor, 500ms};
    // co_await timer.async_wait(async::use_op);
    
    // async::channel<void> stopped{};

    // sam::condition_variable cv{co_await async::this_coro::executor};

    ServiceManager mgr;
    // mgr.stopped_.connect([&cv]() { cout << "stopped\n"; cv.notify_all(); });
    Service srv_1{"1"}, srv_2{"2"}, srv_3{"3"};

    mgr.Add(srv_1);
    mgr.Add(srv_2);
    mgr.Add(srv_3);

    auto start_promise = mgr.Start();

    asio::steady_timer t{co_await async::this_coro::executor, 150ms};
    co_await t.async_wait(async::use_op);

    start_promise.cancel();
    // co_await asio::post(async::use_op);

    +mgr.Stop();

    co_await wait_for(mgr.stopped_);

    cout << "Done\n";
    co_return 0;
}
