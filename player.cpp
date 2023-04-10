
#include <variant>
#include <iostream>

#include <boost/async/main.hpp>
#include <cosm/state_machine.hpp>

namespace {
template <typename Return = void>
using promise = boost::async::promise<Return>;

struct player
{
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};
    struct cd_detected { enum class disk_type { cd, dvd } type; };

    using event = std::variant<play, end_pause, stop, pause, open_close, cd_detected>;

    void start()
    {
        boost::async::spawn(boost::async::this_thread::get_executor(), 
            sm_.run(&player::empty), boost::asio::detached);
    }

    promise<> process_event(event ev)
    {
        co_await sm_.process_event(std::move(ev));
    }

private:
    using sm = cosm::state_machine<player, event>;

    // States
    sm::state stopped()
    {
        std::cout << "stopped\n";
        for (;;)
        {
            auto ev = co_await sm_;
            if (std::get_if<play>(&ev))
            {
                co_await start_playback();
                co_return sm::transition{&player::playing};
            }
            else if (std::get_if<open_close>(&ev))
            {
                co_await open_drawer();
                co_return sm::transition{&player::open};
            }
            else if (std::get_if<stop>(&ev))
                co_return sm::transition{&player::stopped};
        }
    }

    sm::state empty()
    {
        std::cout << "empty\n";
        for (;;)
        {
            event ev = co_await sm_;
            if (std::get_if<open_close>(&ev))
            {
                co_await open_drawer();
                co_return sm::transition{&player::open};
            }
            else if (auto cdd = std::get_if<cd_detected>(&ev))
            {
                if (co_await good_disk_format(*cdd))
                    co_return sm::transition{&player::stopped};    
                else if (auto_start(*cdd))
                    co_return sm::transition{&player::playing};
            }
        }
    }

    sm::state open()
    {
        std::cout << "open\n";
        for (;;)
        {
            if (auto ev = co_await sm_; std::get_if<open_close>(&ev))
            {
                co_await close_drawer();
                co_return sm::transition{&player::empty};
            }
        }
    }

    sm::state playing()
    {
        std::cout << "playing\n";
        for (;;)
        {
            auto ev = co_await sm_;
            if (std::get_if<stop>(&ev))
            {
                co_await stop_playback();
                co_return sm::transition{&player::stopped};
            }
            else if (std::get_if<pause>(&ev))
            {
                co_await pause_playback();
                co_return sm::transition{&player::paused};
            }
            else if (std::get_if<open_close>(&ev))
            {
                co_await stop_and_open();
                co_return sm::transition{&player::open};
            }
        }
    } 

    sm::state paused()
    {
        std::cout << "paused\n";
        for (;;)
        {
            auto ev = co_await sm_;
            if (std::get_if<stop>(&ev))
            {
                co_await stop_playback();
                co_return sm::transition{&player::stopped};
            }
            else if (std::get_if<end_pause>(&ev))
            {
                co_await resume_playback();
                co_return sm::transition{&player::playing};
            }
            else if (std::get_if<open_close>(&ev))
            {
                co_await stop_and_open();
                co_return sm::transition{&player::open};
            }
        }
    }

    // Actions
    promise<> start_playback() { co_return; }
    promise<> stop_playback() { co_return; }
    promise<> pause_playback() { co_return; }
    promise<> resume_playback() { co_return; }
    promise<> stop_and_open() { co_return; }
    promise<> open_drawer() { std::cout << "open_drawer\n"; co_return; }
    promise<> close_drawer() { std::cout << "close_drawer\n"; co_return; }
    promise<> store_cd_info() { co_return; }

    // Guards (async & sync)
    promise<bool> good_disk_format(cd_detected& e)
    {
        if (e.type != cd_detected::disk_type::cd) {
            std::cout << "invalid disk format\n";
            co_return false;
        }
        std::cout << "valid disk format\n";
        co_return true;
    }

    bool auto_start(cd_detected& e)
    {
        return false;
    }

private:
    sm sm_{this};
};
} // unnamed namespace

boost::async::main co_main(int, char**)
{
    player p{};
    p.start();

    co_await p.process_event(player::open_close{});
    co_await p.process_event(player::open_close{});
    co_await p.process_event(player::cd_detected{player::cd_detected::disk_type::dvd});
    co_await p.process_event(player::cd_detected{player::cd_detected::disk_type::cd});
    co_await p.process_event(player::play{});
    co_await p.process_event(player::pause{});
    co_await p.process_event(player::end_pause{});
    co_await p.process_event(player::pause{});
    co_await p.process_event(player::stop{});
    co_await p.process_event(player::stop{});
    co_return 0;
}
