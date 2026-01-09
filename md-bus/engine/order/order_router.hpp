#pragma once 
#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "../bus/bus.hpp"
#include "../common/event.hpp"
#include "../common/log.hpp"

namespace md {

//helper to build Event
inline Event make_event(Topic t, Payload p) {
    Event e;
    e.h.topic = t;
    e.p = std::move(p);
    return e;
}

class OrderRouter {
private :
    EventBus& bus_;
    bool trace_{false};

    SubId sub_tick_{0};
    SubId sub_order_{0};

    mutable std::mutex px_mu;
    std::unordered_map<std::string, double> last_px_;

    std::atomic<uint64_t> next_trade_id_{1};

    //methods-->
    void on_tick(const Event& ev) {
        const Tick* t = std::get_if<Tick>(&ev.p);
        if(!t) return;

        {
            std::scoped_lock lk(px_mu);
            last_px_[t->symbol] = t->pq;
        }
        if(trace_) {
            log_debug("[OrderRouter] tick sym={} px={}", t->symbol, t->pq);
        }
    }
    //optional means that this function will return double or nothing
    std::optional<double> last_price(const std::string& sym) const {
        auto it = last_px_.find(sym);
        //nullopt is no value marker for optional
        if(it == last_px_.end())return std::nullopt;
        return it->second;
    }

    void on_order(const Event& ev) {
        const Order* o = std::get_if<Order>(&ev.p);
        if(!o) return;

        if(!trace_) {
            log_debug("[OrderRouter] ORDER id={} sym={} side={} type={} qty={} px={}",
                      o->order_id, o->symbol,
                      (o->side == Side::Buy ? "BUY" : "SELL"),
                      (o->type == OrderType::Market ? "MKT" : "LMT"),
                      o->qty, o->price);
        }

        if(o->order_id == 0){
            publish_reject(*o, 1001, "order_id=0");
            return;
        }
        if(o->symbol.empty()) {
            publish_reject(*o, 1002, "empty symbol");
            return;
        }
        if(o->qty <= 0) {
            publish_reject(*o, 1003, "qty<=0");
            return;
        }
        if(o->type == OrderType::Limit && o->price <= 0.0) {
            publish_reject(*o, 1004, "limit price<=0");
            return;
        }

        auto px_opt = last_price(o->symbol);
        if(!px_opt.has_value()) {
            publish_reject(*o, 2001, "no last price (need MD_TICK first)");
            return;
        }
        const double mkt_px = *px_opt;

        double fill_px = 0.0;

        if(o->type == OrderType::Market) {
            fill_px = mkt_px;
        } else {
            const bool marketable = (o->side == Side::Buy) ?
            (mkt_px <= o->price) : (mkt_px >= o->price);

            if(!marketable) {
                publish_reject(*o, 2002, "limit not marketable vs last price");
                return;
            }
            fill_px = mkt_px;
        }
        publish_trade(*o, fill_px);
    }

    void publish_trade(const Order& o, double fill_px) {
        Trade tr;
        tr.order_id = o.order_id;
        tr.trade_id = next_trade_id_.fetch_add(1, std::memory_order_relaxed);
        tr.symbol = o.symbol;
        tr.side = o.side;
        tr.qty = o.qty;
        tr.price = fill_px;
        
        bus_.publish(make_event(Topic::TRADE, tr));


        if (trace_) {
            log_debug("[OrderRouter] TRADE tid={} oid={} sym={} qty={} px={}",
                      tr.trade_id, tr.order_id, tr.symbol, tr.qty, tr.price);
        }
    }

    void publish_reject(const Order& o, int code, std::string reason) {
        Reject r;
        r.order_id = o.order_id;
        r.symbol = o.symbol;
        r.code = code;
        r.reason = std::move(reason);

        bus_.publish(make_event(Topic::REJECT, r));

        if (trace_) {
            log_debug("[OrderRouter] REJECT oid={} sym={} code={} reason={}",
                      r.order_id, r.symbol, r.code, r.reason);
        }
    }

public :
    explicit OrderRouter(EventBus& bus, bool trace = false)
        :bus_{bus}, trace_{trace} {
            sub_tick_ = bus_.subscribe(Topic::MD_TICK, [this](const Event& ev){on_tick(ev);});

            sub_order_ = bus_.subscribe(Topic::ORDER, [this](const Event& ev){on_order(ev);});

                log_info("OrderRouter started (trace={})", trace_ ? 1 : 0);
        }
    ~OrderRouter() {
        bus_.unsubscribe(sub_tick_);
        bus_.unsubscribe(sub_order_);
        log_info("OrderRouter stopped");
    }
};


}