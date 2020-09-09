#ifndef TASK_EM_H_
#define TASK_EM_H_

#include <unistd.h> 

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include "common.h"

namespace task {

struct ExecTask
{
    ExecTask() = default;
    ExecTask(uint32_t id, const std::string& exec, const std::string& param, 
        std::vector<uint32_t>& dep): id_(id), exec_(exec), params_(param) {}
    void Dump() const noexcept {
        printf("%d,%s,%s\n", id_, exec_.c_str(), params_.c_str());
    }
    uint32_t id_{0};
    std::string exec_{""};
    std::string params_{""};
    std::vector<uint32_t> depends_;
    int priority_{0};
    bool running_{false};
};

class ExecMgr
{
public:
    virtual ~ExecMgr();
    void Init(const std::string& config);
    void ShutDown();
    int Spawn(ExecTask& t);
    bool Kill(ExecTask& t);
    bool ProcessExist(pid_t pid) const noexcept;
    bool ProcessExist(const ExecTask& t) const noexcept;
    bool Running(uint32_t id) const noexcept;
    ExecTask* GetTask(size_t idx) noexcept;
    size_t TaskCount() const noexcept { return tasks_.size();}
    size_t RunningTaskCount() const noexcept { return runningTasks_.size();}
    pid_t Wait(pid_t pid);

private:
    std::vector<ExecTask> tasks_;
    std::vector<std::pair<uint32_t, pid_t>> runningTasks_;

    bool KillDepends(const ExecTask& t);
    pid_t FindPid(const std::string& exec, const std::string& param) const noexcept;
    const ExecTask& FindTask(pid_t pid) const noexcept;

    static void SigHandler(int sig);

    DECLARE_SINGLETON(ExecMgr);
};

}

#endif // !TASK_EM_H_
