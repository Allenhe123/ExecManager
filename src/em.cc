#include "em.h"
#include "file.h"

#include <sys/types.h>  
#include <sys/wait.h>

#include <thread>
#include <chrono>
#include <csignal>
#include <functional>

#include "manifest.pb.h"
#include "configMgr.h"

namespace task {

ExecMgr::ExecMgr() {
    runningTasks_.reserve(100); 
}

ExecMgr::~ExecMgr() {
    ShutDown();
}

void ExecMgr::SigHandler(int sig) {
    (void)sig;

}

void ExecMgr::Init(const std::string& config) {
    task::proto::TaskList tasklist;
    ConfigMgr confmgr;
    confmgr.Read(config, tasklist);

    for (int j=0; j<tasklist.tasks_size(); j++) {
        auto& tt = tasklist.tasks(j);
        auto vecstr = split(tt.depends(), ',');
        std::vector<uint32_t> vec;
        vec.reserve(vecstr.size());
        for (const auto& str : vecstr)
            vec.push_back(std::stoi(str));
        tasks_.emplace_back(tt.id(), tt.exec(), tt.param(), vec);
        printf("id: %d, exec: %s, param: %s\n", tt.id(), tt.exec().c_str(), tt.param().c_str());
    }

    std::signal(SIGTERM, ExecMgr::SigHandler);
    std::signal(SIGINT, ExecMgr::SigHandler);
    std::signal(SIGCHLD, ExecMgr::SigHandler);

    printf("PID:%d\n", getpid());

    for( auto& t : tasks_ ) {
        if (Running(t.id_)) continue;

        const auto& v = t.depends_;
        for (auto id : v) {
            if (Running(id)) continue;

            for (auto& tsk : tasks_) {
                if (tsk.id_ == id) {
                    if (Spawn(tsk) < 0)
                        printf("spawn %s %s failed\n", tsk.exec_.c_str(), tsk.params_.c_str());
                    break;
                }
            }
        }

        pid_t ret = Spawn(t);
        if (ret < 0) {
            printf("spawn %s %s failed\n", t.exec_.c_str(), t.params_.c_str());
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    printf("running task size: %d\n", runningTasks_.size());
    for (const auto& rtsk : runningTasks_) {
        printf("id: %d, pid: %d\n", rtsk.first, rtsk.second);
    }
}

pid_t ExecMgr::Wait(pid_t pid) {
    int status = 0;
    int ret = waitpid(pid, &status, 0);            
    if (ret == -1) {
        printf("---waitpid %d failed\n", pid);
    } else {
        // if (WIFEXITED(status) && ret == pid)
        //     printf("child %d exit code is: %d.\n", (int)pid, WEXITSTATUS(status));
        // else 
        //     printf("wait child %d failed, exit code is: %d.\n", (int)pid, WEXITSTATUS(status));
    }
    printf("waitpid  return %d\n", ret);
    return ret;
}

void ExecMgr::ShutDown() {
    for (auto ite = tasks_.rbegin(); ite != tasks_.rend(); ite++) {
        uint32_t id = ite->id_;
        const auto depend = ite->depends_;
        if (Running(id)) {
            if (Kill(*ite)) {
                printf("shutdown task: %d and its depends success\n", id);
            } else {
                printf("shutdown task: %d failed\n", id);
            }
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    raise(SIGKILL);
}

int ExecMgr::Spawn(ExecTask& t) {
    int retcode = 0;
    ConfigMgr confmgr;
    confmgr.ParseParam(t.params_);

    pid_t fpid = vfork();
    if (fpid == 0) {
        int ret = execvp(t.exec_.c_str(), confmgr.Param());
        if (ret < 0) {
            perror( "execvp error " );
            retcode = -1;
        }
    } else if (fpid > 0) {
        runningTasks_.push_back(std::make_pair(t.id_, fpid));
        retcode = fpid;
        t.running_ = true;
    } else {
        perror("vfork error ");
        retcode = -1;
    }

    return retcode;
}

bool ExecMgr::Kill(ExecTask& t) {
    bool ret = true;
    pid_t pid = FindPid(t.exec_, t.params_);
    if (pid == -1) {
        ret &= true;
    } else {
        if (ProcessExist(pid)) {
            printf("kill pid: %d\n", pid);
            if (kill(pid, SIGKILL) == 0) {
                if (Wait(pid) != -1) {
                    for (auto ite = runningTasks_.begin(); ite != runningTasks_.end(); ite++) {
                        if (ite->second == pid) {
                            runningTasks_.erase(ite);
                            break;
                        }
                    }
                    t.running_ = false;

                    ret &= KillDepends(t);
                } else {
                    ret &= false;    
                }
            } else {
                printf("send SIGKILL to %d failed\n", pid);
                ret &= false;
            }
        }
    }
    return ret;
}


bool ExecMgr::KillDepends(const ExecTask& t) {
    bool ret = true;
    std::vector<uint32_t> ids;
    for (auto i : t.depends_) {
        bool haveOtherDepends = false;

        for (const auto& tsk : tasks_) {
            if (!tsk.running_) continue;

            if (tsk.id_ == i) {
                haveOtherDepends = true;
                break;
            }

            for (auto j : tsk.depends_) {
                if (j == i) {
                    haveOtherDepends = true;
                    break;
                }
            }

            if (haveOtherDepends) break;
        }

        if (!haveOtherDepends)
            ids.push_back(i);
    }

    for (auto k : ids) {
        for (auto & tsk : tasks_) {
            if (k == tsk.id_) {
                ret &=Kill(tsk);
            }
        }
    }
    return ret;
}

bool ExecMgr::ProcessExist(pid_t pid) const noexcept {
    if (kill(pid, 0) == -1) return false;
    return true;
}

bool ExecMgr::ProcessExist(const ExecTask& t) const noexcept {
    pid_t pid = FindPid(t.exec_, t.params_);
    if (pid == -1) {
        return false;
    } else {
        return ProcessExist(pid);
    }
}

pid_t ExecMgr::FindPid(const std::string& exec, const std::string& param) const noexcept {
    uint32_t id = 0;
    for (const auto& t : tasks_) {
        t.Dump();
        if (t.exec_ == exec && ((t.params_ == param) || (t.params_.empty() && param.empty()) ))
            id = t.id_;
            break;
    }
    if (id  != 0) {
        for (const auto& pr : runningTasks_) {
            if (id == pr.first) {
                return pr.second;
            }
        }
    }
    return -1;
}

const ExecTask& ExecMgr::FindTask(pid_t pid) const noexcept {
    uint32_t id = 0;
    for (const auto& pr : runningTasks_) {
        if (pid == pr.second) {
            id = pr.first;
            break;
        }
    }

    for (const auto& t : tasks_) {
        if (t.id_ == id) 
            return std::reference_wrapper<const ExecTask>(t);
    }
    ExecTask tt;
    return std::move(tt);
}

bool ExecMgr::Running(uint32_t id) const noexcept {
    bool running = false;
    for (const auto& pr : runningTasks_) {
        if (pr.first == id && ProcessExist(pr.second)) {
            running = true;
            break; 
        }
    }
    return running;
}

ExecTask* ExecMgr::GetTask(size_t idx) noexcept {
    if (idx >=0 && idx < tasks_.size()) return  &tasks_[idx];
    return nullptr;
}


}