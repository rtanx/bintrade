#pragma once

#include <bintrade/core/config.hpp>
#include <bintrade/core/export.hpp>

#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace bintrade {

/// Initialize the global bintrade logger with the given configuration.
/// Must be called at most once, before any logging.  Thread-safe
/// (uses std::call_once internally).  Subsequent calls are no-ops.
///
/// If never called, the logger is lazily initialized with LogConfig
/// defaults on first use of logger().
BINTRADE_API void init_logging(const LogConfig& config = LogConfig{});

/// Return the global bintrade logger.  Lazily initialized with defaults
/// if init_logging() was never called.  Thread-safe.
[[nodiscard]] BINTRADE_API std::shared_ptr<spdlog::logger> logger();

/// Replace the logger's sink with a user-provided one.  This is the
/// customization point for users who want to route bintrade log output
/// to their own infrastructure (file, network, custom sink).
///
/// The new sink inherits the current log level and pattern.
/// Thread-safe (swaps sink under spdlog's internal mutex).
BINTRADE_API void set_log_sink(spdlog::sink_ptr sink);

/// Convenience: set the log level at runtime.
BINTRADE_API void set_log_level(LogLevel level);

}  // namespace bintrade
