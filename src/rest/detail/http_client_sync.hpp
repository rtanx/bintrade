#pragma once

// Synchronous convenience wrappers for executing async awaitables in a
// blocking context.
//
// These utilities are for callers that cannot or choose not to use co_await
// (e.g. examples, scripts, non-HFT glue code).  They must NOT be used on the
// WebSocket I/O thread or any latency-critical hot path.
//
// Typical usage with HttpClient:
//
//   HttpClient client(config);
//   auto resp = bintrade::rest::detail::sync_await(
//       client.get_executor(), client.async_get("/api/v3/ping"));
//
// The sync get/post/del methods on HttpClient already follow this pattern
// internally; these helpers are provided for cases where callers build their
// own awaitables on top of the transport.

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/use_future.hpp>

namespace bintrade::rest::detail {

/// Block the calling thread until the awaitable completes on the given
/// executor, then return its result (or re-throw any exception).
///
/// Template parameters:
///   Executor -- any Boost.Asio executor (e.g. io_context::executor_type).
///   T        -- the value type carried by the awaitable.
///
/// IMPORTANT: The calling thread MUST NOT be the thread that drives the
/// executor.  If exec is HttpClient::get_executor(), the HttpClient already
/// runs its io_context on a dedicated background thread, so any other thread
/// (e.g. main, strategy) may safely call sync_await.
template <typename Executor, typename T>
T sync_await(Executor&& exec, boost::asio::awaitable<T> aw) {
    return boost::asio::co_spawn(std::forward<Executor>(exec), std::move(aw), boost::asio::use_future).get();
}

}  // namespace bintrade::rest::detail
