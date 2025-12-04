#pragma once 

#include <vector>
#include "strategy.hpp"

namespace md {

class MultiStrategy : public IStrategy {
private : 
    std::vector<IStrategy*> strategies_;
public : 
    MultiStrategy() = default ;
    void add_strategy(IStrategy* s) {
        if(s) {
            strategies_.push_back(s);
        }
    }
    //here auto would also work
    //but we use auto* to make it only use pointers
    void on_tick(const Tick& t, const Event& e) override {
        for(auto* s : strategies_) {
            s->on_tick(t, e);
        }
    }

    void on_log(const std::string& msg, const Event& e) override {
        for(auto* s : strategies_) {
            s->on_log(msg, e);
        }
    }

    void on_heartbeat(const Event& e) override {
        for(auto* s : strategies_) {
            s->on_heartbeat(e);
        }
    }
};

}