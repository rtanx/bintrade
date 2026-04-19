#pragma once

// TypedUserStream<Handler> -- high-performance user data stream with
// compile-time callback dispatch.
//
// The Handler type provides one or both of:
//   void on_order_update(const models::Order&)
//   void on_account_update(const models::AccountInfo&)
//
// Only methods present on the handler are compiled; missing methods
// are silently skipped via if-constexpr.
//
// Example:
//   struct MyHandler {
//       void on_order_update(const bintrade::models::Order& o) noexcept { ... }
//   };
//   MyHandler h;
//   bintrade::ws::TypedUserStream<MyHandler> stream(h, creds);
//   stream.start();

#include <bintrade/ws/detail/ws_parse.hpp>
#include <bintrade/ws/user_stream.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace bintrade::ws {

template <typename Handler>
class TypedUserStream : public UserStream {
public:
    TypedUserStream(Handler& handler, Credentials credentials, RestConfig rest_config = RestConfig{}, WebSocketConfig ws_config = WebSocketConfig{})
        : UserStream(std::move(credentials), std::move(rest_config), std::move(ws_config)), handler_(handler) {}

protected:
    void do_dispatch(std::string_view raw_json) override {
        try {
            auto j = nlohmann::json::parse(raw_json);
            const auto event_type = j.value("e", "");

            if (event_type == "executionReport") {
                if constexpr (requires { handler_.on_order_update(std::declval<const models::Order&>()); }) {
                    handler_.on_order_update(detail::parse_order_update(j));
                }
            } else if (event_type == "outboundAccountPosition") {
                if constexpr (requires { handler_.on_account_update(std::declval<const models::AccountInfo&>()); }) {
                    handler_.on_account_update(detail::parse_account_update(j));
                }
            }
        } catch (const std::exception& /*e*/) {  // NOLINT(bugprone-empty-catch)
        }
    }

private:
    Handler& handler_;
};

}  // namespace bintrade::ws
