#pragma once
#include<atomic>
#include<functional>
#include<memory>
#include<string>
#include<thread>
#include <mutex>
#include<unordered_map>
#include<vector>

#include "../common/bounded_queue.hpp"
#include "../common/event.hpp"
#include "../common/metrics.hpp"


namespace md {

using Callback = std::function<void(const Event&)>;
using SubId = uint64_t;

class EventBus {
private:
    struct SubSlot {
        Topic t {Topic::MD_TICK};
        std::unique_ptr<BoundedQueue<Event>> q;
        std::thread worker;
        std::atomic<bool>run{true};
        Callback cb;
    };

    void reactor_loop();

    std::unique_ptr<BoundedQueue<Event>> ingress_; //producer - > reactor
    std::thread reactor_;
    std::atomic<bool> run_{true};

    // rounting and bookkeeping (for subscriptions)
    std::mutex mu_;
    std::unordered_map<SubId, std::unique_ptr<SubSlot>> subs_;
    std::unordered_map<SubId, std::unique_ptr<SubSlot>> all_subs_;
    const size_t per_sub_cap_;

    // sequence + ids
    std::atomic<uint64_t> seq_{0};
    std::atomic<uint64_t> next_id_{1}; // remember ot initialize this from 1st id

    std::atomic<uint64_t> published_{0};
    std::atomic<uint64_t> ingress_popped_{0};

    static constexpr size_t kMaxTopics = 8;
    std::array<std::atomic<uint64_t>, kMaxTopics> topic_counts_{0}; // array to keep 
    //track of the topic counts

    std::atomic<bool> perf_enabled_{true};
    std::atomic<bool> reactor_trace_{false};

    uint64_t perf_start_ns_ = 0;
    uint64_t perf_end_ns_   = 0;

    mutable std::mutex perf_mu_;
    Log2Histogram<48> latency_ns_hist_;

public:
    explicit EventBus(size_t ingress_cap = 65536, size_t per_sub_cap = 65536);

    ~EventBus();

    //declaration
    SubId subscribe(Topic T, Callback cb);
    SubId subscribe_all(Callback cb);
    void unsubscribe(SubId id);

    // (non blocking) enqueue in ingress_ and return
    bool publish(Event e);
    bool publish_preserve(Event e);
    void stop(); // gracefully shutdown

    void print_stats() const;

    PerfSnapshot perf_snapshot() const;

    void set_perf_enabled(bool on) {perf_enabled_.store(on, std::memory_order_relaxed);}
    
    void set_reactor_trace(bool on) {reactor_trace_.store(on, std::memory_order_relaxed);}

};
}


