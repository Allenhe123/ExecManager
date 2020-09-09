#include <iostream>
#include "../src/em.h"
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    task::ExecMgr::Instance()->Init("sample.conf");
    std::this_thread::sleep_for(std::chrono::seconds(15));
    bool ret = task::ExecMgr::Instance()->Kill(*task::ExecMgr::Instance()->GetTask(0));
    if (!ret) std::cout << "kill process failed" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(30));
    task::ExecMgr::Instance()->ShutDown();

    return 0;
}