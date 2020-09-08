#include "em.h"
#include "file.h"

#include <sys/types.h>  
#include <sys/wait.h>

#include <thread>
#include <chrono>
#include <csignal>
#include <functional>

#include "manifest.pb.h"

namespace task {

ExecMgr::ExecMgr() {
    pids_.reserve(100);
    runningTasks_.reserve(100); 
}

ExecMgr::~ExecMgr() {
    ShutDown();
}

void ExecMgr::SigHandler(int sig) {
    (void)sig;

}

void ExecMgr::Init(const std::string& config) {
    config_ = config;

    task::proto::TaskList tasklist;
    if (file_exist(config_) && apollo::cyber::common::GetProtoFromFile(config_, &tasklist)) {
        std::cout << "load conf successfully" << std::endl;
    } else {
        std::cout << "load conf failed" << std::endl;
       return;
    }

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

    printf("pids size: %d\n", pids_.size());
    for (auto i : pids_) 
        printf("pid: %d\n", i);
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
    int sszie = 0;
    auto param = ParseParam(t.params_, sszie);
    pid_t fpid = vfork();
    if (fpid == 0) {
        int ret = execvp(t.exec_.c_str(), param);
        if (ret < 0) {
            perror( "execvp error " );
            retcode = -1;
        }
    } else if (fpid > 0) {
        pids_.push_back(fpid);
        runningTasks_.push_back(std::make_pair(t.id_, fpid));
        retcode = fpid;
        t.running_ = true;
    } else {
        perror("vfork error ");
        retcode = -1;
    }

    for (int i=0; i<sszie; i++) {
        if (param[i] != nullptr) {
            delete []param[i]; 
            param[i] = nullptr;
        }
    }
    delete []param;
    param = nullptr;

    return retcode;
}

char** ExecMgr::ParseParam(const std::string& param, int& ssize) {
    auto vec = split(param, ' ');
    ssize = vec.size();
    if (ssize == 0) ssize = 1;
    char** p = new char*[ssize];
    int idx = 0;
    for (const auto& str : vec) {
        p[idx] = new char[str.size()];
        std::memcpy(p[idx], str.c_str(), str.size());
        idx++;
    }
    return p;
}

int ExecMgr::Kill(ExecTask& t) {
    int ret_code = 0;
    pid_t pid = FindPid(t.exec_, t.params_);
    if (pid == -1) {
        ret_code = 0;
    } else {
        if (ProcessExist(pid)) {
            printf("kill pid: %d\n", pid);
            if (kill(pid, SIGKILL) == 0) {
                if (Wait(pid) != -1) {
                    for (auto ite = pids_.begin(); ite != pids_.end(); ite++) {
                        if (*ite == pid) {
                            pids_.erase(ite);
                            break;
                        }
                    }
                    for (auto ite = runningTasks_.begin(); ite != runningTasks_.end(); ite++) {
                        if (ite->second == pid) {
                            runningTasks_.erase(ite);
                            break;
                        }
                    }
                    t.running_ = false;

                    ret_code = 1;
                    
                    KillDepends(t);
                }
          
            } else {
                printf("send SIGKILL to %d failed\n", pid);
                ret_code = -1;
            }
        }
    }
    return ret_code;
}

// bool ExecMgr::Kill(pid_t pid) {
//     if (ProcessExist(pid)) {
//         printf("kill pid: %d\n", pid);
//         if (kill(pid, SIGKILL) == 0) {
//             pid_t ret_id = Wait(pid);
//             for (auto ite = pids_.begin(); ite != pids_.end(); ite++) {
//                 if (*ite == pid) {
//                     pids_.erase(ite);
//                     break;
//                 }
//             }
//             for (auto ite = runningTasks_.begin(); ite != runningTasks_.end(); ite++) {
//                 if (ite->second == pid) {
//                     runningTasks_.erase(ite);
//                     break;
//                 }
//             }
//             return true;           
//         } else {
//             printf("send SIGKILL to %d failed\n", pid);
//             return false;
//         }
//     }
// }


bool ExecMgr::KillDepends(const ExecTask& t) {
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
                Kill(tsk);
            }
        }
    }
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