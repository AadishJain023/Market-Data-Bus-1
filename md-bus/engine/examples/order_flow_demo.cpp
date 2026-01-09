#include "../bus/bus.hpp"
#include "../common/log.hpp"
#include "../common/event.hpp"
#include "../order/order_router.hpp"
#include <thread>

using namespace md;

static Event tick_ev(const std::string& sym, double px, uint32_t qty = 1) {
    Tick t;
    t.symbol = sym;
    t.pq = px;
    t.qty = qty;
    return make_event(Topic::MD_TICK, t);
}

static Event order_ev(uint64_t oid, const std::string& sym, Side side, OrderType type, int qty, double px=0.0) {
    Order o;
    o.order_id = oid;
    o.symbol   = sym;
    o.side     = side;
    o.type     = type;
    o.qty      = qty;
    o.price    = px;
    return make_event(Topic::ORDER, o);
}

int main() {
    EventBus bus(1024, 1024);

    // Print trades/rejects coming out of the router
    auto sub_tr = bus.subscribe(Topic::TRADE, [](const Event& ev){
        const auto* tr = std::get_if<Trade>(&ev.p);
        if (!tr) return;
        log_info("[TRADE] oid={} tid={} sym={} side={} qty={} px={}",
                 tr->order_id, tr->trade_id, tr->symbol,
                 (tr->side == Side::Buy ? "BUY" : "SELL"),
                 tr->qty, tr->price);
    });

    auto sub_rj = bus.subscribe(Topic::REJECT, [](const Event& ev){
        const auto* r = std::get_if<Reject>(&ev.p);
        if (!r) return;
        log_info("[REJECT] oid={} sym={} code={} reason={}",
                 r->order_id, r->symbol, r->code, r->reason);
    });

    OrderRouter router(bus, /*trace=*/true);

    // Feed a tick so router has a last price
    bus.publish(tick_ev("NIFTY", 22500.0));

    // Market BUY: should fill
    bus.publish(order_ev(1, "NIFTY", Side::Buy, OrderType::Market, 10));

    // Limit BUY at 22400: last is 22500 -> not marketable -> reject
    bus.publish(order_ev(2, "NIFTY", Side::Buy, OrderType::Limit, 10, 22400.0));

    // Limit BUY at 22600: last 22500 <= 22600 -> marketable -> fill
    bus.publish(order_ev(3, "NIFTY", Side::Buy, OrderType::Limit, 10, 22600.0));

    // Update tick and sell limit example
    bus.publish(tick_ev("NIFTY", 22520.0));

    // Limit SELL at 22550: last 22520 < 22550 -> not marketable -> reject
    bus.publish(order_ev(4, "NIFTY", Side::Sell, OrderType::Limit, 5, 22550.0));

    // Limit SELL at 22500: last 22520 >= 22500 -> marketable -> fill
    bus.publish(order_ev(5, "NIFTY", Side::Sell, OrderType::Limit, 5, 22500.0));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    bus.unsubscribe(sub_tr);
    bus.unsubscribe(sub_rj);

    bus.stop();
    bus.print_stats();
    return 0;
}
