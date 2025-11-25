// engine/examples/hello_bus.cpp
#include <fmt/core.h>
#include "../bus/bus.hpp"
#include "../common/event.hpp"

int main() {
  md::EventBus bus(/*ingress*/1024, /*per-sub*/1024);

    auto sub_ticks = bus.subscribe(md::Topic::MD_TICK, [](const md::Event e){
        if(std::holds_alternative<md::Tick>(e.p)){
            const auto&t = std::get<md::Tick>(e.p);
            fmt::print("seq{} sym ={} pq{}\n",e.h.seq, t.symbol, t.pq);
        }
    });

    auto subs_logs = bus.subscribe(md::Topic::LOG, [](const md::Event e){
        if(std::holds_alternative<std::string>(e.p)){
            const auto&msg = std::get<std::string>(e.p);
            fmt::print("seq {} msg {}\n", e.h.seq, msg);
        }
    });

    auto sub_hb = bus.subscribe(md::Topic::HEARTBEAT, [](const 
    md::Event e){
        fmt::print("[MON ] seq={} topic={}\n",
               e.h.seq,
               static_cast<int>(e.h.topic));
    });

    // md::SimpleTimer nb_timer(
    //     md::Duration(200),[&bus]{

    //     }
    // );
  for (int i = 0; i < 5; ++i) {
    md::Tick t{
        .symbol = "NIFTY", 
        .pq = 22500.0 + i, 
        .qty = static_cast<uint32_t>(100 + i)
    };
    md::Header h{};
    h.topic = md::Topic::MD_TICK;
    bus.publish(md::Event{ .h = h, .p = t });

    md::Header log_h{};
    log_h.topic = md::Topic::LOG;
    std::string msg = " Published Ticks " + std::to_string(i);
    bus.publish(md::Event{.h = log_h, .p = msg});
  }


  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  bus.unsubscribe(sub_ticks);
  bus.unsubscribe(subs_logs);
  bus.stop();
  return 0;
}
