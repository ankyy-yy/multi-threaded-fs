#pragma once

namespace mtfs::threading {

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queueMutex);

        // Don't allow enqueueing after stopping the pool
        if(stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        tasks.emplace([task](){ (*task)(); });
    }
    
    condition.notify_one();
    return res;
}

template<class F, class... Args>
void ThreadPool::enqueue_detached(F&& f, Args&&... args) {
    auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    
    {
        std::unique_lock<std::mutex> lock(queueMutex);

        if(stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        tasks.emplace([task](){ task(); });
    }
    
    condition.notify_one();
}

} // namespace mtfs::threading
