#include <iostream>
#include "gtest/gtest.h"
#include "../src/em.h"

#include <thread>
#include <chrono>

TEST (EM_TEST, Test_0) {
    task::ExecMgr::Instance()->Init("sample.conf");
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // task::ExecMgr::Instance()->Kill(*task::ExecMgr::Instance()->GetTask(0));
    std::this_thread::sleep_for(std::chrono::seconds(20));
    // task::ExecMgr::Instance()->ShutDown();
}

int main(int argc, char** argv) {
    // ::testing::InitGoogleTest(&argc, argv);
    // return RUN_ALL_TESTS();

    task::ExecMgr::Instance()->Init("sample.conf");
    std::this_thread::sleep_for(std::chrono::seconds(15));
    auto p = task::ExecMgr::Instance()->GetTask(0);
    if (p != nullptr)
        task::ExecMgr::Instance()->Kill(*task::ExecMgr::Instance()->GetTask(0));
    std::this_thread::sleep_for(std::chrono::seconds(30));
    task::ExecMgr::Instance()->ShutDown();

    return 0;
}