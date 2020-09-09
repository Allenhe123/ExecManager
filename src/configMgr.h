#ifndef CONFIG_MGR_H_
#define CONFIG_MGR_H_

#include <string>
#include "manifest.pb.h"

namespace task {

class ConfigMgr 
{
public:
    ConfigMgr() = default;
    ~ConfigMgr();
    ConfigMgr(const ConfigMgr&) = delete;
    ConfigMgr& operator = (ConfigMgr&) = delete;

    void Read(const std::string& conf, task::proto::TaskList& tasklist) const;
    void ParseParam(const std::string& param);
    char** Param() const noexcept { return param_; }

private:
    char** param_ = nullptr;
    size_t size_{0};

};


}

#endif