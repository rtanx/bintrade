#include <bintrade/core/logger.hpp>

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <mutex>

namespace bintrade {

namespace {

std::shared_ptr<spdlog::logger> g_logger;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::once_flag g_init_flag;                // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

auto to_spdlog_level(LogLevel level) noexcept -> spdlog::level::level_enum {
    return static_cast<spdlog::level::level_enum>(static_cast<int>(level));
}

void do_init(const LogConfig& config) {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    if (config.use_async) {
        spdlog::init_thread_pool(config.async_queue_size, config.async_thread_count);
        g_logger =
            std::make_shared<spdlog::async_logger>("bintrade", std::move(sink), spdlog::thread_pool(), spdlog::async_overflow_policy::overrun_oldest);
    } else {
        g_logger = std::make_shared<spdlog::logger>("bintrade", std::move(sink));
    }

    g_logger->set_level(to_spdlog_level(config.level));
    g_logger->set_pattern(config.pattern);

    spdlog::register_logger(g_logger);
}

}  // namespace

void init_logging(const LogConfig& config) {
    std::call_once(g_init_flag, do_init, config);
}

auto logger() -> std::shared_ptr<spdlog::logger> {
    std::call_once(g_init_flag, do_init, LogConfig{});
    return g_logger;
}

void set_log_sink(spdlog::sink_ptr sink) {
    auto log = logger();
    log->flush();  // Drain async queue before modifying sinks.
    log->sinks().clear();
    log->sinks().push_back(std::move(sink));
}

void set_log_level(LogLevel level) {
    logger()->set_level(to_spdlog_level(level));
}

}  // namespace bintrade
