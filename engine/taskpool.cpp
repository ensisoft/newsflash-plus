// Copyright (c) 2010-2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include <newsflash/config.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <map>
#include "taskpool.h"
#include "taskcore.h"
#include "buffer.h"

namespace newsflash
{

class taskpool::thread 
{
private:
    enum class action {
        none, prepare, cancel, finalize, flush, enqueue, exit_thread
    };
    struct args {
        action act;
        std::size_t task;
    };

public:
    thread() 
    {}

    void start()
    {
        thread_.reset(new std::thread(std::bind(&thread::run, this)));
    }
    void stop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        work_.push({action::exit_thread});
        cond_.notify_one();
        lock.unlock();

        thread_->join();
        thread_.reset();
    }
    void insert(std::size_t id, std::unique_ptr<taskcore> task)
    {
        auto* priv  = new struct task;
        priv->id    = id;
        priv->task  = std::move(task);
        priv->error = false;
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.insert(std::make_pair(id, priv));
    }
    void prepare(std::size_t id)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        work_.push({action::prepare, id});
        cond_.notify_one();
    }


private:
    struct task {
        std::unique_ptr<taskcore> task;
        std::size_t id;
        bool error;
    };

private:
    void run() 
    {
        for (;;)
        {
            args a = {action::none};
            for (;;) {
                std::unique_lock<std::mutex> lock(mutex_);
                if (work_.empty())
                    cond_.wait(lock);
                else if (!work_.empty())
                {
                    a = work_.front();
                    work_.pop();
                    break;
                }
            }
            if (a.act == action::exit_thread)
                return;

            auto* task = find_task(a.task);            
            if (task->error)
                continue;

            try 
            {
                switch (a.act)
                {
                    case action::prepare:
                        task->task->prepare();
                        break;
                    case action::cancel:
                        task->task->cancel();
                        break;
                    case action::flush:
                        task->task->flush();
                        break;
                    case action::finalize:
                        task->task->finalize();
                        break;
                    default:
                        assert(0);
                        break;
                }
            }
            catch (const std::exception& e)
            {
                task->error = true;
            }
        }
    }
    task* find_task(std::size_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tasks_.find(id);
        return it->second;
    }
private:
    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::map<std::size_t, task*> tasks_;    
    std::queue<args> work_;
};

taskpool::taskpool(int num_threads)
{
    for (int i=0; i<num_threads; ++i)
    {
        std::unique_ptr<thread> th(new thread);
        th->start();
        threads_.push_back(std::move(th));
    }
}

taskpool::~taskpool()
{
    for (auto& th : threads_)
    {
        th->stop();
    }
}

void taskpool::insert(std::size_t id, std::unique_ptr<taskcore> task)
{
    auto& thread = threads_[id % threads_.size()];
    thread->insert(id, std::move(task));
}

void taskpool::prepare(std::size_t id)
{
    auto& thread = threads_[id % threads_.size()];
    thread->prepare(id);
}

void taskpool::enqueue(std::size_t id, std::unique_ptr<buffer> buffer)
{

}

} // newsflash

