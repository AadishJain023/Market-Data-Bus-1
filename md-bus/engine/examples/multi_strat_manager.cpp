#include <fmt/core.h>
#include <thread>
#include <chrono>

#include "../bus/bus.hpp"
#include "../common/event.hpp"
#include "../common/log.hpp"
#include "../replay/replay.hpp"
#include "../bar/bar_builder.hpp"

#include "../strategy/accounting.hpp"
#include "../strategy/strategy.hpp"
#include "../strategy/bar_window.hpp"
#include "../strategy/bar_momentum.hpp"
#include "../strategy/strategy_manager.hpp"

int main() {
    using namespace std::chrono_literals;

    md::EventBus bus(1024, 1024);
    static constexpr uint64_t NS_PER_10MS = 10'000'000ULL;
    md::BarBuilder bar_builder(bus, NS_PER_10MS);

    auto sub_bars = bus.subscribe(md::Topic::BAR_1S, [](const md::Event& e){
        if(!std::holds_alternative<md::Bar>(e.p)) return;
        const auto& b = std::get<md::Bar>(e.p);
        fmt::print("[BAR] sym={} o={} h={} l={} c={} v={}\n",
                   b.symbol, b.open, b.high, b.low, b.close, b.volume);
    });

    //account for each strategy
    md::Account acct_mom1;
    md::Account acct_mom2;

    md::BarMomentumStrategy strat_mom1(
        acct_mom1,
        "NIFTY",   // symbol
        2,         // window_size
        0.05,      // momentum_threshold
        1
    );

    md::BarMomentumStrategy strat_mom2 (
        acct_mom2,
        "NIFTY",   // symbol
        3,         // window_size
        0.10,      // momentum_threshold
        1
    );

    md::StrategyManager mgr(bus);
    mgr.add_strategy(&strat_mom1);
    mgr.add_strategy(&strat_mom2);
    mgr.start();

    md::EventReplay replayer("logs/md_events.log");
    md::ReplayFilter filter;
    filter.filter_by_topic = true;
    filter.topic = md::Topic::MD_TICK;
    filter.filter_by_symbol = false;
    replayer.set_filter(filter);
    replayer.replay_fast(bus);
    std::this_thread::sleep_for(200ms);

    bar_builder.flush_all();
    std::this_thread::sleep_for(100ms);

    strat_mom1.finalize();
    strat_mom2.finalize();
    mgr.finalize_all();  // already handled by above 2 lines

    mgr.stop();
    bus.unsubscribe(sub_bars);

    bus.stop();
    bus.print_stats();

    fmt::print("\n=== BarMomentum Strategy 1 (acct_mom1) ===\n");
    acct_mom1.print_summary();
    acct_mom1.dump_trades_csv("trades_barmomentum_1.csv");

    fmt::print("\n=== BarMomentum Strategy 2 (acct_mom2) ===\n");
    acct_mom2.print_summary();
    acct_mom2.dump_trades_csv("trades_barmomentum_2.csv");

    fmt::print("[INFO] dumped bar-momentum trades to trades_barmomentum_1.csv and trades_barmomentum_2.csv\n");

    return 0;

}
