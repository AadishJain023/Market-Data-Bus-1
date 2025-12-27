#pragma once 

#include <vector>
#include <memory>

#include "../bus/bus.hpp"
#include "../common/event.hpp"
#include "../common/log.hpp"
#include "strategy.hpp"

namespace md{

/**
 * StrategyManager
 * ---------------
 * Single subscriber to EventBus that fans out events to multiple strategies.
 *
 * - You register one or more IStrategy*.
 * - Manager subscribes via subscribe_all().
 * - For each incoming Event, it dispatches to appropriate callbacks:
 *     MD_TICK   -> on_tick()
 *     LOG       -> on_log()
 *     HEARTBEAT -> on_heartbeat()
 *     BAR_1S    -> on_bar()
 * - finalize_all() calls strategy->finalize() on all.
 */

class StrategyManager {
private :
    EventBus& bus_;
    bool started_{false};
    SubId sub_all_{0};
    std::vector<IStrategy*> strategies_;

    void on_event(const Event& e) {
        switch(e.h.topic) {
            case Topic::MD_TICK : {
                if(!std::holds_alternative<Tick>(e.p)) {
                    log_warn("StrategyManager: MD_TICK event without Tick payload (seq={})",
                             e.h.seq);
                    return;
                }
                const Tick& t = std::get<Tick>(e.p);
                for(auto *strat : strategies_) {
                    strat->on_tick(t, e);
                }
                break;
            }
            case Topic::LOG: {
                if(!std::holds_alternative<std::string>(e.p)) {
                    return;
                }
                const auto& msg = std::get<std::string>(e.p);
                for(auto* strat : strategies_) {
                    strat->on_log(msg, e);
                }
                break;
            }
            case Topic::HEARTBEAT: {
                for(auto* strat : strategies_) {
                    strat->on_heartbeat(e);
                }
                break;
            }
            case Topic::BAR_1S: {
                if(!std::holds_alternative<Bar>(e.p)) {
                    log_warn("StrategyManager: BAR_1S event without Bar payload (seq={})",
                             e.h.seq);
                    return;
                }
                const Bar&b = std::get<Bar>(e.p);
                for(auto* strat : strategies_) {
                    strat->on_bar(b, e);
                }
                break;
            }
            case Topic::BAR_1M: {
                if(!std::holds_alternative<Bar>(e.p)) {
                    log_warn("StrategyManager: BAR_1M event without Bar payload (seq={})",
                             e.h.seq);
                    return;
                }
                const Bar&b = std::get<Bar>(e.p);
                for(auto* strat : strategies_) {
                    strat->on_bar(b, e);
                }
                break;
            }
            default: {
                break;
            }
        }
    }
public:
    explicit StrategyManager(EventBus& bus)
        : bus_ {bus} {}
    
    //cannot use copy constructor
    StrategyManager(const StrategyManager&) = delete;
    StrategyManager& operator = (const StrategyManager&) = delete;

    void add_strategy(IStrategy* strat) {
        if(!strat)return;
        strategies_.push_back(strat);
        log_info("StrategyManager: added strategy '{}'", strat->name());
    }

    void start() {
        if(started_) return;
        started_ = true;
        sub_all_ = bus_.subscribe_all([this](const Event& e){
            this->on_event(e);
        });
        log_info("StrategyManager: started with {} strategies", strategies_.size());
    }

    void stop() {
        if(!started_)return;
        started_ = false;

        if(sub_all_ != 0) {
            bus_.unsubscribe(sub_all_);
            sub_all_ = 0;
        }

        log_info("StrategyManager: stopped");
    }

    void finalize_all() {
        for(auto* strat : strategies_) {
            if(!strat) continue;
            log_info("StrategyManager: finalizing strategy '{}'", strat->name());
            strat->finalize();
        }
    }

    ~StrategyManager() {
        if(started_ && sub_all_ != 0) {
            bus_.unsubscribe(sub_all_);
        }
    }

};

}

