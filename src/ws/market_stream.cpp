#include "detail/ws_parse.hpp"

#include <bintrade/core/error.hpp>
#include <bintrade/core/logger.hpp>
#include <bintrade/core/spsc_queue.hpp>
#include <bintrade/ws/market_stream.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <functional>
#include <string>
#include <string_view>
#include <thread>

namespace bintrade::ws {

// -----------------------------------------------------------------------
// QueueState -- SPSC queue + consumer thread for DispatchMode::Queued.
// -----------------------------------------------------------------------
struct MarketStream::QueueState {
    static constexpr std::size_t k_queue_capacity = 4'096;

    SpscQueue<std::string, k_queue_capacity> queue;
    std::thread consumer_thread;
    std::atomic<bool> running{false};
};

// -----------------------------------------------------------------------
// Special members (need QueueState complete type).
// -----------------------------------------------------------------------
MarketStream::MarketStream(WebSocketConfig config) : Client(std::move(config)) {}

MarketStream::~MarketStream() {
    stop_consumer();
}

MarketStream::MarketStream(MarketStream&&) noexcept = default;
MarketStream& MarketStream::operator=(MarketStream&&) noexcept = default;

// -----------------------------------------------------------------------
// Dispatch mode
// -----------------------------------------------------------------------
void MarketStream::set_dispatch_mode(DispatchMode mode) {
    if (subscribed_) {
        throw ValidationException("set_dispatch_mode must be called before subscribe");
    }
    dispatch_mode_ = mode;
}

// -----------------------------------------------------------------------
// Dispatch installation (inline vs queued).
// -----------------------------------------------------------------------
void MarketStream::install_dispatch(std::function<void(std::string_view)> dispatch_fn) {
    if (dispatch_mode_ == DispatchMode::Queued) {
        if (!queue_state_) {
            queue_state_ = std::make_unique<QueueState>();
        }
        set_message_callback([state = queue_state_.get()](std::string_view msg) { (void)state->queue.try_push(std::string(msg)); });
        start_consumer(std::move(dispatch_fn));
    } else {
        set_message_callback(std::move(dispatch_fn));
    }
}

// -----------------------------------------------------------------------
// Consumer thread lifecycle
// -----------------------------------------------------------------------
void MarketStream::start_consumer(std::function<void(std::string_view)> dispatch_fn) {
    if (!queue_state_) {
        queue_state_ = std::make_unique<QueueState>();
    }
    queue_state_->running.store(true, std::memory_order_relaxed);

    queue_state_->consumer_thread = std::thread([state = queue_state_.get(), dispatch_fn = std::move(dispatch_fn)] {
        while (state->running.load(std::memory_order_acquire)) {
            if (auto msg = state->queue.try_pop()) {
                dispatch_fn(*msg);
            } else {
                std::this_thread::yield();
            }
        }
        // Drain remaining messages after shutdown signal.
        while (auto msg = state->queue.try_pop()) {
            dispatch_fn(*msg);
        }
    });
}

void MarketStream::stop_consumer() {
    if (queue_state_ && queue_state_->running.load(std::memory_order_relaxed)) {
        queue_state_->running.store(false, std::memory_order_release);
        if (queue_state_->consumer_thread.joinable()) {
            queue_state_->consumer_thread.join();
        }
    }
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
namespace {

std::string lower(std::string s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}  // namespace

// -----------------------------------------------------------------------
// Subscribe methods
// -----------------------------------------------------------------------
void MarketStream::subscribe_trades(const Symbol& symbol, const TradeCallback& callback) {
    subscribed_ = true;
    auto stream_name = lower(symbol) + "@trade";
    bintrade::logger()->info("MarketStream subscribing to {}", stream_name);

    auto dispatch_fn = [callback](std::string_view msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            if (j.value("e", "") == "trade") {
                callback(detail::parse_trade_stream(j));
            }
        } catch (const std::exception& ex) {
            bintrade::logger()->warn("MarketStream dispatch error: {}", ex.what());
        }
    };

    install_dispatch(std::move(dispatch_fn));
    connect("/ws/" + stream_name);
}

void MarketStream::subscribe_klines(const Symbol& symbol, std::string_view interval, const KlineCallback& callback) {
    subscribed_ = true;
    auto stream_name = lower(symbol) + "@kline_" + std::string(interval);
    bintrade::logger()->info("MarketStream subscribing to {}", stream_name);

    auto dispatch_fn = [callback](std::string_view msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            if (j.value("e", "") == "kline") {
                callback(detail::parse_kline_stream(j));
            }
        } catch (const std::exception& ex) {
            bintrade::logger()->warn("MarketStream dispatch error: {}", ex.what());
        }
    };

    install_dispatch(std::move(dispatch_fn));
    connect("/ws/" + stream_name);
}

void MarketStream::subscribe_ticker(const Symbol& symbol, const TickerCallback& callback) {
    subscribed_ = true;
    auto stream_name = lower(symbol) + "@ticker";
    bintrade::logger()->info("MarketStream subscribing to {}", stream_name);

    auto dispatch_fn = [callback](std::string_view msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            if (j.value("e", "") == "24hrTicker") {
                callback(detail::parse_ticker_stream(j));
            }
        } catch (const std::exception& ex) {
            bintrade::logger()->warn("MarketStream dispatch error: {}", ex.what());
        }
    };

    install_dispatch(std::move(dispatch_fn));
    connect("/ws/" + stream_name);
}

void MarketStream::subscribe_depth(const Symbol& symbol, const OrderBookCallback& callback) {
    subscribed_ = true;
    auto stream_name = lower(symbol) + "@depth";
    bintrade::logger()->info("MarketStream subscribing to {}", stream_name);

    auto dispatch_fn = [callback](std::string_view msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            if (j.value("e", "") == "depthUpdate") {
                callback(detail::parse_depth_stream(j));
            }
        } catch (const std::exception& ex) {
            bintrade::logger()->warn("MarketStream dispatch error: {}", ex.what());
        }
    };

    install_dispatch(std::move(dispatch_fn));
    connect("/ws/" + stream_name);
}

}  // namespace bintrade::ws
