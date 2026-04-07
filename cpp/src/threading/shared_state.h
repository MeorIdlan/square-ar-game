#pragma once
// Thread-safe latest-value holder and worker thread base

#include <mutex>
#include <optional>
#include <atomic>
#include <thread>
#include <functional>

namespace sag {

// Holds the latest value written by a producer. Consumer takes it atomically.
template<typename T>
class LatestValue {
public:
    void set(T value) {
        std::lock_guard lock(mtx_);
        value_ = std::move(value);
    }

    std::optional<T> take() {
        std::lock_guard lock(mtx_);
        if (!value_) return std::nullopt;
        auto v = std::move(*value_);
        value_.reset();
        return v;
    }

    bool has_value() const {
        std::lock_guard lock(mtx_);
        return value_.has_value();
    }

private:
    mutable std::mutex mtx_;
    std::optional<T>   value_;
};

// Simple worker thread that runs a loop function until stopped
class WorkerThread {
public:
    using LoopFn = std::function<void()>;

    void start(LoopFn loop_fn) {
        running_.store(true);
        thread_ = std::thread([this, fn = std::move(loop_fn)]() {
            while (running_.load()) {
                fn();
            }
        });
    }

    void stop() {
        running_.store(false);
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    bool is_running() const { return running_.load(); }

    ~WorkerThread() { stop(); }

private:
    std::atomic<bool> running_{false};
    std::thread       thread_;
};

} // namespace sag
