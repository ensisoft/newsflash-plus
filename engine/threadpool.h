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

#pragma once

#include "newsflash/config.h"

#include <functional>
#include <memory>
#include <queue>
#include <atomic>
#include <cstddef>

namespace newsflash
{
    class action;

    class ThreadPool
    {
    public:
        class Thread;

        // initialize the pool with num_threads.
        ThreadPool(std::size_t initial_pool_size);
       ~ThreadPool();

        void AddMainThread(bool pooled, bool private_thread);

        void Submit(std::unique_ptr<action> act)
        {
            Submit(act.get());
            act.release();
        }

        // submit an action to the ThreadPool for any thread to execute specific
        // the thread affinity constraint set on the action.
        void Submit(action* act);

        // submit work to the specific thread
        void Submit(action* act, Thread* thread);

        // wait for all actions to be completed before returning.
        void WaitAllActions();

        // shutdown the ThreadPool. will block and join all the threads.
        void Shutdown();

        // allocate a private thread.
        // returns a handle to the private thread.
        Thread* AllocatePrivateThread();

        // detach and return the thread to the ThreadPool.
        // the thread will continue to perform all the actions queued
        // in it's queue and then become eligible for using again
        void DetachPrivateThread(Thread* thread);

        std::size_t GetNumPendingActions() const;

        // callback to be invoked when an action has been completed
        using OnActionDone = std::function<void (action*)>;
        void SetCallback(const OnActionDone& callback);

        void RunMainThreads();

    private:
        struct State;
        class RealThread;
        class MainThread;

    private:
        std::shared_ptr<State> state_;
        std::vector<std::unique_ptr<Thread>> pooled_threads_;
        std::vector<std::unique_ptr<Thread>> private_threads_;
        std::size_t round_robin_ = 0;
    };

} // newsflash
