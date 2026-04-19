#include <bintrade/core/logger.hpp>

#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/ostream_sink.h>

#include <sstream>
#include <string>
#include <thread>

namespace bintrade::test {

// ---------------------------------------------------------------------------
// Test fixture: installs a synchronous ostream sink so assertions can
// inspect log output deterministically.
// ---------------------------------------------------------------------------
class LoggerTest : public ::testing::Test {
protected:
    std::ostringstream log_output_;
    std::shared_ptr<spdlog::sinks::ostream_sink_mt> test_sink_;

    void SetUp() override {
        // Use synchronous logger for deterministic test assertions.
        LogConfig config;
        config.use_async = false;
        init_logging(config);

        // Replace the default sink with an ostream sink for capture.
        test_sink_ = std::make_shared<spdlog::sinks::ostream_sink_mt>(log_output_);
        set_log_sink(test_sink_);

        // Simple pattern for test assertions (no timestamps).
        logger()->set_pattern("[%l] %v");

        // Capture all levels.
        set_log_level(LogLevel::Trace);
    }

    void TearDown() override {
        // Drain all pending async messages, then replace the ostream sink
        // with a null sink so the async thread pool never writes to the
        // fixture's (about-to-be-destroyed) ostringstream.
        logger()->flush();
        set_log_sink(std::make_shared<spdlog::sinks::null_sink_mt>());
        set_log_level(LogLevel::Info);
    }

    // Flush and return everything captured so far.
    std::string captured() {
        logger()->flush();
        return log_output_.str();
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------
TEST_F(LoggerTest, LoggerIsInitialized) {
    auto log = logger();
    ASSERT_NE(log, nullptr);
    EXPECT_EQ(log->name(), "bintrade");
}

TEST_F(LoggerTest, SinkCapturesOutput) {
    logger()->info("hello world");
    EXPECT_NE(captured().find("hello world"), std::string::npos);
}

TEST_F(LoggerTest, LevelFilteringWorks) {
    set_log_level(LogLevel::Warn);
    logger()->info("should not appear");
    logger()->warn("should appear");
    auto output = captured();
    EXPECT_EQ(output.find("should not appear"), std::string::npos);
    EXPECT_NE(output.find("should appear"), std::string::npos);
}

TEST_F(LoggerTest, SetLogSinkReplacesOldSink) {
    // Write to original sink.
    logger()->info("to original");
    logger()->flush();
    EXPECT_NE(log_output_.str().find("to original"), std::string::npos);

    // Replace with a new sink.
    std::ostringstream new_output;
    auto new_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(new_output);
    set_log_sink(new_sink);
    logger()->set_pattern("[%l] %v");

    logger()->info("to new sink");
    logger()->flush();
    EXPECT_NE(new_output.str().find("to new sink"), std::string::npos);

    // The original sink should NOT have received the new message.
    EXPECT_EQ(log_output_.str().find("to new sink"), std::string::npos);

    // Restore fixture sink before test exits -- new_output is local and
    // will be destroyed when this scope ends, so the logger must not
    // reference it past this point.
    set_log_sink(test_sink_);
    logger()->set_pattern("[%l] %v");
}

TEST_F(LoggerTest, SetLogLevelAtRuntime) {
    set_log_level(LogLevel::Error);
    logger()->warn("suppressed");
    logger()->error("visible");
    auto output = captured();
    EXPECT_EQ(output.find("suppressed"), std::string::npos);
    EXPECT_NE(output.find("visible"), std::string::npos);
}

TEST_F(LoggerTest, LogLevelOffProducesNoOutput) {
    set_log_level(LogLevel::Off);
    auto before = log_output_.str().size();
    for (int i = 0; i < 1'000; ++i) {
        logger()->debug("hot path message {}", i);
    }
    auto output = captured();
    EXPECT_EQ(output.size(), before);
}

TEST_F(LoggerTest, AllLevelsProduceOutput) {
    logger()->trace("t");
    logger()->debug("d");
    logger()->info("i");
    logger()->warn("w");
    logger()->error("e");
    logger()->critical("c");
    auto output = captured();
    EXPECT_NE(output.find("[trace]"), std::string::npos);
    EXPECT_NE(output.find("[debug]"), std::string::npos);
    EXPECT_NE(output.find("[info]"), std::string::npos);
    EXPECT_NE(output.find("[warning]"), std::string::npos);
    EXPECT_NE(output.find("[error]"), std::string::npos);
    EXPECT_NE(output.find("[critical]"), std::string::npos);
}

TEST_F(LoggerTest, LoggerIsSingleton) {
    auto a = logger();
    auto b = logger();
    EXPECT_EQ(a.get(), b.get());
}

TEST_F(LoggerTest, LoggerIsThreadSafe) {
    constexpr int num_threads = 4;
    constexpr int msgs_per_thread = 100;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t] {
            for (int i = 0; i < msgs_per_thread; ++i) {
                logger()->info("thread {} msg {}", t, i);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    // No crash is the primary assertion. Verify at least some output.
    auto output = captured();
    EXPECT_NE(output.find("thread"), std::string::npos);
}

}  // namespace bintrade::test
