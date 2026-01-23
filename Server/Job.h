#pragma once

class Job
{
    using JobFunc = function<void()>;
public:

    Job(JobFunc func)
        : job_func(std::move(func))
    {
    }

    template<typename PacketType>
    Job(std::shared_ptr<Session> session
        , function<void(shared_ptr<Session>, PacketType)> func
        , PacketType pkt)
    {
        job_func = [session, func, pkt]() mutable {
            func(session, pkt);
            };
    }

    Job(const Job& other)
        : job_func(other.job_func)
    {
    }

    Job& operator=(const Job& other)
    {
        job_func = other.job_func;
        return *this;
    }

    Job(Job&& other) noexcept
        : job_func(std::move(other.job_func))
    {
    }

    Job& operator=(Job&& other) noexcept
    {
        job_func = std::move(other.job_func);
        return *this;
    }

    void Execute()
    {
        if (job_func)
            job_func();
    }

private:
    JobFunc job_func;
};

