#include "Task.h"

namespace igtk
{
    Task::Task(size_t id, TaskManager& taskManager) :
        id_{ id },
        taskManager_{ taskManager }
    {

    }

    Task::~Task()
    {
        running_ = false;
        inUse_ = true;
        callback_ = []() {};
        trigger();
        if (worker_ != nullptr)
        {
            worker_->join();
        }
    }

    void Task::work()
    {
        while (running_)
        {
            //std::lock_guard<std::mutex> lockGuard{ mutex_ };

            std::unique_lock<std::mutex> uniqueLock{ mutex_ };
            taskConditionVariable_.wait(uniqueLock, [&]()
            {
                return triggered_ == true;
            });
            lastLockedCode_ = 2;
            triggered_ = false;
            callback_();
            inUse_ = false;
            TaskManager::TaskAttorney::notify(taskManager_);
        }
    }

    void Task::setWork(const std::function<void()>& callback)
    {
        std::lock_guard<std::mutex> lockGuard{ mutex_ };
        lastLockedCode_ = 0;
        callback_ = callback;
        inUse_ = true;
        
        if (worker_ == nullptr)
        {
            worker_ = std::make_unique<std::thread>( [this]()->void {this->work();} );
        }
    }

    void Task::trigger()
    {
        std::lock_guard<std::mutex> lockGuard{ mutex_ };
        lastLockedCode_ = 1;
        triggered_ = true;
        taskConditionVariable_.notify_one();
    }


    size_t TaskManager::execute(const std::function<void()>& workload)
    {
        std::lock_guard<std::mutex> lockGuard{ executeCriticalSection_ };
        lockCount_++;
        bool taskAvailable = false;
        size_t taskId = -1;
        for (auto& task : tasks_)
        {
            if (task->isInUse() == false)
            {
                taskAvailable = true;
                taskId = task->id_;
                break;
            }
        }

        if (!taskAvailable)
        {
            taskId = tasks_.size();
            tasks_.emplace_back(new Task(taskId, *this));
        }

        tasks_[taskId]->setWork(workload);
        tasks_[taskId]->trigger();
        lockCount_--;
        return taskId;
    }

    void TaskManager::setRunning(bool running)
    {
        std::lock_guard<std::mutex> lockGuard{ taskManagerMutex_ };
        lockCount_++;
        running_ = running;
        notifiedCount_++;
        taskManagerConditionVariable_.notify_one();
        lockCount_--;
    }

    void TaskManager::setOnNotifyCallback(const std::function<void()>& callback)
    {
        std::lock_guard<std::mutex> lockGuard{taskManagerMutex_};
        lockCount_++;
        onNotifyCallback_.reset(new std::function<void()>{ callback });
        running_ = true;
        if (taskManagerWorkerThread_ == nullptr)
        {
            taskManagerWorkerThread_.reset(new std::thread{ [this]() {this->loop();} });
        }
        lockCount_--;
    }

    void TaskManager::loop()
    {
        while (running_)
        {
            std::unique_lock<std::mutex> uniqueLock{ taskManagerMutex_ };
            
            taskManagerConditionVariable_.wait(uniqueLock, [&]()
            {
                return notifiedCount_ > 0;
            });
            lockCount_++;
            if (onNotifyCallback_ != nullptr)
            {
                (*onNotifyCallback_)();
            }
            notifiedCount_--;
            lockCount_--;
        }
    }
}