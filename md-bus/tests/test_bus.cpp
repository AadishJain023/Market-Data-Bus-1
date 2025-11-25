#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include "../engine/bus/bus.hpp"
#include "../engine/common/event.hpp"

using namespace md;

TEST(Bus, PublishesAndReceives) {
  EventBus bus(16, 16);
  std::atomic<int> count{0};

  auto id = bus.subscribe(Topic::MD_TICK, [&](const Event& e){
    if (std::holds_alternative<Tick>(e.p)) {
      count.fetch_add(1, std::memory_order_relaxed);
    }
  });

  Header h{};
  h.topic = Topic::MD_TICK;

  for (int i = 0; i < 5; ++i) {
    Tick t{.symbol="X", .pq=1.0 + i, .qty=10};
    bus.publish(Event{ .h = h, .p = t });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(count.load(), 5);

  bus.unsubscribe(id);
  bus.stop();
}

TEST(Bus, MultipleSubscribersIndependent) {
  EventBus bus(64, 64);
  std::atomic<int> tick_count{0};
  std::atomic<int> log_count{0};

  auto tick_sub = bus.subscribe(Topic::MD_TICK, [&](const Event& e){
    if (std::holds_alternative<Tick>(e.p)) {
      // simulate slow subscriber
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      tick_count.fetch_add(1, std::memory_order_relaxed);
    }
  });

  auto log_sub = bus.subscribe(Topic::LOG, [&](const Event& e){
    if (std::holds_alternative<std::string>(e.p)) {
      log_count.fetch_add(1, std::memory_order_relaxed);
    }
  });

  Header th{};
  th.topic = Topic::MD_TICK;
  Header lh{};
  lh.topic = Topic::LOG;

  for (int i = 0; i < 10; ++i) {
    Tick t{.symbol="X", .pq=1.0 + i, .qty=10};
    bus.publish(Event{ .h = th, .p = t });

    std::string msg = "log " + std::to_string(i);
    bus.publish(Event{ .h = lh, .p = msg });
  }

  // Give time for workers to process
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  EXPECT_EQ(tick_count.load(), 10);
  EXPECT_EQ(log_count.load(), 10);

  bus.unsubscribe(tick_sub);
  bus.unsubscribe(log_sub);
  bus.stop();
}