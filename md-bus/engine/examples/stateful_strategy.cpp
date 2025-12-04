#include <chrono>
#include <fmt/core.h>
#include <thread>

#include "../bus/bus.hpp"
#include "../common/event.hpp"
#include "../common/log.hpp"
#include "../replay/replay.hpp"
#include "../strategy/strategy.hpp"
#include "../strategy/runner.hpp"

// Stateful trading strategy:
// - Goes LONG when price crosses above threshold
// - Exits when price falls back below threshold
// - Tracks trades, realized/unrealized PnL, and simple MFE/MAE-style stats

class TradingThresholdStrategy : public md::IStrategy {
private :
    double threshold_;
    int qty_; // assuming you can buy integers only

    bool in_position_{false};
    double entry_price_{0.0};

    double realized_pnl_{0.0};
    double unrealized_pnl_{0.0};
    double max_upside_pnl_{0.0}; //best unrealized pnl seen
    double max_drawdown_pnl_{0.0}; // worst unrealized pnl seen

    int trades_ = 0;
public :
    explicit TradingThresholdStrategy(double threshold, int qty = 1)
        :threshold_{threshold}, qty_{qty} {}

    void on_tick(const md::Tick& t, const md::Event& e) override {
        const double px = t.pq;
        if(in_position_) {
            unrealized_pnl_ = (px - entry_price_) * qty_;
            if(unrealized_pnl_ > max_upside_pnl_){
                max_upside_pnl_ = unrealized_pnl_;
            }
            if(unrealized_pnl_ < max_drawdown_pnl_) {
                max_drawdown_pnl_ = unrealized_pnl_;
            }
        }
         if(!in_position_ && px > threshold_) {
            in_position_  = true;
            entry_price_  = px;
            unrealized_pnl_ = 0.0;
            trades_++;

            fmt::print("[STRAT] ENTER LONG seq={} sym={} pq={} threshold={} qty={}\n",
                       e.h.seq, t.symbol, px, threshold_, qty_);
            return;
        }
        if(in_position_ && px < threshold_) {
            double trade_pnl = (px - entry_price_) * qty_;
            realized_pnl_ += trade_pnl;
            fmt::print("[STRAT] EXIT  LONG seq={} sym={} pq={} entry={} qty={} trade_pnl={}\n",
                       e.h.seq, t.symbol, px, entry_price_, qty_, trade_pnl);

            // After exit, reset state
            in_position_    = false;
            entry_price_    = 0.0;
            unrealized_pnl_ = 0.0;
            return;
        }
    }
    void on_log(const std::string& msg, const md::Event& e) override {
        // Optional: observe LOG events
        fmt::print("[STRAT-LOG] seq={} msg={}\n", e.h.seq, msg);
    }

    void on_heartbeat(const md::Event& e) override {
        // Optional: observe HEARTBEAT events
        fmt::print("[STRAT-HB] seq={} topic={}\n",
                   e.h.seq, static_cast<int>(e.h.topic));
    }

    void print_summary() const {
        fmt::print("\n==== TradingThresholdStrategy Summary ====\n");
        fmt::print("  threshold         = {}\n",   threshold_);
        fmt::print("  qty               = {}\n",   qty_);
        fmt::print("  trades            = {}\n",   trades_);
        fmt::print("  realized_pnl      = {}\n",   realized_pnl_);
        fmt::print("  unrealized_pnl    = {}\n",   unrealized_pnl_);
        fmt::print("  max_upside_pnl    = {}\n",   max_upside_pnl_);
        fmt::print("  max_drawdown_pnl  = {}\n",   max_drawdown_pnl_);
        fmt::print("  in_position       = {}\n",   (in_position_ ? "true" : "false"));
        if (in_position_) {
            fmt::print("  entry_price       = {}\n", entry_price_);
        }
        fmt::print("=========================================\n");
    }
};

int main () {
    using namespace std::chrono_literals;
    md::EventBus bus(1024, 1024);
    TradingThresholdStrategy strat(22500.0, 1);
    {
        md::StrategyRunner runner(bus, strat);
        md::EventReplay replayer("logs/md_events.log");
        md::ReplayFilter f;
        f.filter_by_topic = true;
        f.topic = md::Topic::MD_TICK;

        f.filter_by_symbol = true;
        f.symbol = "NIFTY";

        replayer.set_filter(f);
        replayer.replay_realtime(bus);
        std::this_thread::sleep_for(200ms);
    }

    strat.print_summary();
    bus.stop();
    bus.print_stats();
    return 0;
}