#include "threading/thread_pool.hpp"
#include <algorithm>

namespace mtfs::threading {

ThreadPool::ThreadPool(size_t numThreads) {
    // Ensure at least 2 threads
    numThreads = std::max(size_t(2), numThreads);
    
    for(size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    
    condition.notify_all();
    
    for(std::thread &worker: workers) {
        if(worker.joinable()) {
            worker.join();
        }
    }
}

size_t ThreadPool::getQueueSize() const {
    std::unique_lock<std::mutex> lock(const_cast<std::mutex&>(queueMutex));
    return tasks.size();
}

bool ThreadPool::isBusy() const {
    return getQueueSize() > 0 || getActiveThreads() > 0;
}

void ThreadPool::pause() {
    paused = true;
}

void ThreadPool::resume() {
    paused = false;
    condition.notify_all();
}

void ThreadPool::waitForAll() {
    std::unique_lock<std::mutex> lock(queueMutex);
    finished.wait(lock, [this]() {
        return tasks.empty() && activeThreads.load() == 0;
    });
}

void ThreadPool::resize(size_t newSize) {
    if (newSize == workers.size()) return;
    
    if (newSize < workers.size()) {
        // Shrink pool
        size_t removeCount = workers.size() - newSize;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        
        // Join excess threads
        for (size_t i = 0; i < removeCount; ++i) {
            if (workers.back().joinable()) {
                workers.back().join();
            }
            workers.pop_back();
        }
        
        stop = false;
    } else {
        // Grow pool
        size_t addCount = newSize - workers.size();
        for (size_t i = 0; i < addCount; ++i) {
            workers.emplace_back(&ThreadPool::workerThread, this);
        }
    }
}

void ThreadPool::workerThread() {
    for(;;) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            
            condition.wait(lock, [this]() {
                return stop || (!paused && !tasks.empty());
            });
            
            if(stop && tasks.empty()) {
                return;
            }
            
            if(!tasks.empty()) {
                task = std::move(tasks.front());
                tasks.pop();
                activeThreads++;
            }
        }

        if(task) {
            try {
                task();
            } catch(...) {
                // Log error but don't let exceptions kill the thread
            }
            
            activeThreads--;
            finished.notify_all();
        }
    }
}

// Global thread pool implementation
std::unique_ptr<ThreadPool> GlobalThreadPool::instance = nullptr;
std::mutex GlobalThreadPool::instanceMutex;

ThreadPool& GlobalThreadPool::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance) {
        instance = std::make_unique<ThreadPool>();
    }
    return *instance;
}

void GlobalThreadPool::initialize(size_t numThreads) {
    std::lock_guard<std::mutex> lock(instanceMutex);
    instance = std::make_unique<ThreadPool>(numThreads);
}

void GlobalThreadPool::shutdown() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    instance.reset();
}

} // namespace mtfs::threading
