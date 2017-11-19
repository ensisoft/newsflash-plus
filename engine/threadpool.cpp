// Copyright (c) 2010-2015 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "newsflash/config.h"

#include <condition_variable>
#include <mutex>
#include <functional>
#include <algorithm>
#include <cassert>
#include <thread>

#include "threadpool.h"
#include "action.h"
#include "minidump.h"

namespace newsflash
{
struct ThreadPool::State {
    std::atomic<std::size_t> queue_size;

    OnActionDone callback;

    State() : queue_size(0)
    {}
};

class ThreadPool::Thread
{
public:
    virtual ~Thread() = default;
    virtual void Run()
    {}
    virtual void Submit(action* act) = 0;
    virtual void Shutdown() = 0;

    virtual bool InPrivateUse() const
    { return false; }

    virtual void SetPrivateUse(bool on_off)
    { }
private:
};

class ThreadPool::RealThread : public ThreadPool::Thread
{
public:
    RealThread(std::shared_ptr<State> state) : state_(state)
    {
        std::lock_guard<std::mutex> lock(mutex);
        run_loop = true;
        thread.reset(new std::thread(std::bind(&RealThread::ThreadSehMain, this)));
    }
    virtual void Submit(action* act) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(act);
        cond.notify_one();
    }
    virtual void Shutdown() override
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            run_loop = false;
            cond.notify_one();
        }
        thread->join();
    }
    virtual bool InPrivateUse() const  override
    { return in_use_; }

    virtual void SetPrivateUse(bool on_off) override
    { in_use_ = on_off;}

private:
    void ThreadMain()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (!run_loop)
                return;
            else if (queue.empty())
            {
                cond.wait(lock);
                continue;
            }

            assert(!queue.empty());

            action* next = queue.front();
            queue.pop();
            lock.unlock();

            next->perform();

            state_->callback(next);
            state_->queue_size--;
        }
    }

    void ThreadSehMain()
    {
        SEH_BLOCK(ThreadMain();)
    }

private:
    bool in_use_ = false;

private:
    std::mutex mutex;
    std::condition_variable cond;
    std::unique_ptr<std::thread> thread;
    std::queue<action*> queue;
    bool run_loop = false;
    std::shared_ptr<State> state_;
};

class ThreadPool::MainThread : public ThreadPool::Thread
{
public:
    MainThread(std::shared_ptr<State> state) : state_(state)
    {}

    virtual void Run() override
    {
        while (!queue_.empty())
        {
            action* top = queue_.front();
            queue_.pop();
            top->perform();
            state_->callback(top);
            state_->queue_size--;
        }
    }
    virtual void Submit(action* act) override
    { queue_.push(act); }

    virtual void Shutdown() override
    {}

    virtual bool InPrivateUse() const override
    { return in_use_; }

    virtual void SetPrivateUse(bool on_off) override
    { in_use_ = on_off; }

private:
    bool in_use_ = false;
    std::queue<action*> queue_;
    std::shared_ptr<State> state_;
};

ThreadPool::ThreadPool(std::size_t initial_pool_size)
{
    state_ = std::make_shared<State>();

    // creating a threadpool with zero threads is allowed.
    // it mostly makes sense only when the main thread is also added the pool
    // thus turning the whole system into a single threaded system
    // this is mostly convenient for debugging purposes.

    for (std::size_t i=0; i<initial_pool_size; ++i)
    {
        std::unique_ptr<ThreadPool::Thread> thread(new RealThread(state_));
        pooled_threads_.push_back(std::move(thread));
    }
}

ThreadPool::~ThreadPool()
{
    Shutdown();
}

void ThreadPool::AddMainThread(bool pooled, bool private_thread)
{
    // this is important here, if there are actions that need
    // go to specific thread's queue (in order to maintain) order
    // calling AddMainThread could mess this up since the
    // round robin scheduling would then fail.
    // hence it's currently disallowed to call this
    // method if there are pending actions.
    ASSERT(state_->queue_size == 0);

    if (pooled)
    {
        std::unique_ptr<ThreadPool::Thread> thread(new MainThread(state_));
        pooled_threads_.push_back(std::move(thread));
    }

    if (private_thread)
    {
        std::unique_ptr<ThreadPool::Thread> thread(new MainThread(state_));
        private_threads_.push_back(std::move(thread));
    }


}

void ThreadPool::Submit(action* act)
{

    const auto num_threads  = pooled_threads_.size();
    const auto affinity = act->get_affinity();
    Thread* thread = nullptr;

    if (affinity == action::affinity::any_thread)
    {
        thread = pooled_threads_[round_robin_ % num_threads].get();
        round_robin_++;
    }
    else
    {
        const auto act_id = act->get_owner();
        thread = pooled_threads_[act_id % num_threads].get();
    }
    thread->Submit(act);

    state_->queue_size++;
}

void ThreadPool::Submit(action* act, ThreadPool::Thread* thread)
{
    thread->Submit(act);

    state_->queue_size++;
}

void ThreadPool::WaitAllActions()
{
    // this is quick and dirty waiting
    while (state_->queue_size > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void ThreadPool::Shutdown()
{
    for (auto& thread : pooled_threads_)
    {
        thread->Shutdown();
    }
    for (auto& thread : private_threads_)
    {
        thread->Shutdown();
    }
    pooled_threads_.clear();
    private_threads_.clear();
}

ThreadPool::Thread* ThreadPool::AllocatePrivateThread()
{
    for (std::size_t i=0; i<private_threads_.size(); ++i)
    {
        auto& thread = private_threads_[i];
        if (!thread->InPrivateUse())
        {
            thread->SetPrivateUse(true);
            return thread.get();
        }
    }

    std::unique_ptr<ThreadPool::Thread> thread(new RealThread(state_));
    thread->SetPrivateUse(true);

    auto* handle = thread.get();

    private_threads_.push_back(std::move(thread));
    return handle;
}

void ThreadPool::DetachPrivateThread(ThreadPool::Thread* thread)
{
    thread->SetPrivateUse(false);
}

std::size_t ThreadPool::GetNumPendingActions() const
{ return state_->queue_size; }

void ThreadPool::SetCallback(const OnActionDone& callback)
{ state_->callback = callback; }

void ThreadPool::RunMainThreads()
{
    for (auto& thread : pooled_threads_)
    {
        thread->Run();
    }
    for (auto& thread : private_threads_)
    {
        thread->Run();
    }
}

} // newsflash
