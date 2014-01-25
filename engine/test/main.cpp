
#include <newsflash/engine/engine.h>
#include <newsflash/engine/listener.h>
#include <mutex>
#include <condition_variable>

class testapp : public newsflash::listener
{
public:
    testapp() : engine_(this), shutdown_(false)
    {}

    virtual void on_error(const newsflash::listener::error& e) override
    {}

    virtual void on_shutdown() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        cond_.notify_one();
    }

    virtual void on_complete(const newsflash::listener::file& file) override
    {
    }

    virtual void on_complete(const newsflash::listener::filegroup& batch) override
    {}

    void main()
    {
        while (true)
        {

        }
        
        // request shutdown.
        engine_.shutdown();

        // wait for signal
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [&]{ return !shutdown_; });
    }

private:
    std::mutex mutex_;
    std::condition_variable cond_;

    newsflash::engine engine_;
    bool shutdown_;
};


int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: test hostname:port [ssl]\n");
        return 0;
    }

    testapp test;
    test.main();

    return 0;
}
