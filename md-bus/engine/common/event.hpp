#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <chrono>

namespace md {
    
enum class Topic : uint8_t {
    LOG = 0,
    MD_TICK = 1,
    HEARTBEAT = 2, 
    BAR_1S = 3,
    BAR_1M = 4,
    ORDER = 5,
    TRADE = 6,
    REJECT = 7,
    BOOK_UPDATE = 8,
    RISK_ALERT = 9
};

struct Bar {
    std::string symbol;
    double open{0.0};
    double close{0.0};
    double high{0.0};
    double low{0.0};
    int volume{0};
    uint64_t start_ts_ns{0};
    uint64_t end_ts_ns{0};
};

struct Header {
    uint64_t seq{0};
    Topic topic{Topic::MD_TICK};
    uint64_t ts_ns{0};
    uint64_t t_pub_ns{0};
};

struct Tick {
    std::string symbol;
    double pq{0.0};
    uint32_t qty{0};
};

struct Heartbeat {
    uint64_t t_ms{0};   // optional; you can set from clock
};

enum class Side : uint8_t { Buy = 0, Sell = 1 };
enum class OrderType : uint8_t { Market = 0, Limit = 1 };

struct Order {
    uint64_t order_id{0};
    std::string symbol;
    Side side{Side::Buy};
    OrderType type{OrderType::Market};

    int qty{0};
    double price{0.0};          // for limit (or fill reference)
};

struct Trade {
    uint64_t order_id{0};
    uint64_t trade_id{0};
    std::string symbol;
    Side side{Side::Buy};

    int qty{0};
    double price{0.0};
};

struct Reject {
    uint64_t order_id{0};
    std::string symbol;
    int code{0};
    std::string reason;
};

struct BookUpdate {
    std::string symbol;
    double best_bid{0.0};
    double best_ask{0.0};
    int bid_qty{0};
    int ask_qty{0};
};

struct RiskAlert {
    std::string symbol;
    int code{0};
    std::string reason;
};


using Payload = std::variant<
    std::monostate,
    Tick,
    std::string,
    Bar,
    Heartbeat,
    Order,
    Trade,
    Reject,
    BookUpdate,
    RiskAlert
>;

struct Event {
    Header h;
    Payload p;
};



}