#include "configMgr.h"
#include "file.h"
#include "common.h"
#include <cstring>

namespace task {
ConfigMgr::~ConfigMgr() {
    for (int i=0; i<size_; i++) {
        if (param_[i] != nullptr) {
            delete []param_[i]; 
            param_[i] = nullptr;
        }
    }
    delete []param_;
    param_ = nullptr;
}

void ConfigMgr::Read(const std::string& conf, task::proto::TaskList& tasklist) const {
    if (file_exist(conf) && apollo::cyber::common::GetProtoFromFile(conf, &tasklist)) {
        std::cout << "load conf: " << conf << " successfully" << std::endl;
    } else {
        std::cout << "load conf: " << conf << " failed" << std::endl;
    }
}

void ConfigMgr::ParseParam(const std::string& param) {
    auto vec = split(param, ' ');
    size_ = vec.size();
    if (size_ == 0) size_ = 1;
    param_ = new char*[size_];
    int idx = 0;
    for (const auto& str : vec) {
        param_[idx] = new char[str.size()];
        std::memcpy(param_[idx], str.c_str(), str.size());
        idx++;
    }
}

}