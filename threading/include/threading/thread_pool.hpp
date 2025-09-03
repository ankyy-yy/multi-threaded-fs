#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <chrono>

namespace mtfs::threading {

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Submit a task and get a future for the result
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // Submit a task without return value (fire and forget)
    template<class F, class... Args>
    void enqueue_detached(F&& f, Args&&... args);

    // Get pool statistics
    size_t getThreadCount() const { return workers.size(); }
    size_t getQueueSize() const;
    size_t getActiveThreads() const { return activeThreads.load(); }
    bool isBusy() const;

    // Pool management
    void pause();
    void resume();
    void waitForAll();
    void resize(size_t newSize);

private:
    // Worker threads
    std::vector<std::thread> workers;
    
    // Task queue
    std::queue<std::function<void()>> tasks;
    
    // Synchronization
    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable finished;
    
    // State management
    std::atomic<bool> stop{false};
    std::atomic<bool> paused{false};
    std::atomic<size_t> activeThreads{0};
    
    void workerThread();
};

// Async file operation types
enum class AsyncOpType {
    READ,
    WRITE,
    COPY,
    MOVE,
    DELETE,
    COMPRESS,
    DECOMPRESS,
    BACKUP
};

// Async operation result
template<typename T>
struct AsyncResult {
    std::future<T> future;
    AsyncOpType operation;
    std::string path;
    std::chrono::steady_clock::time_point startTime;
    
    AsyncResult(std::future<T>&& fut, AsyncOpType op, const std::string& p)
        : future(std::move(fut)), operation(op), path(p), 
          startTime(std::chrono::steady_clock::now()) {}
};

// Global thread pool instance
class GlobalThreadPool {
public:
    static ThreadPool& getInstance();
    static void initialize(size_t numThreads = std::thread::hardware_concurrency());
    static void shutdown();

private:
    static std::unique_ptr<ThreadPool> instance;
    static std::mutex instanceMutex;
};

} // namespace mtfs::threading

// Template implementation
#include "thread_pool.tpp"
