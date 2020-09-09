#ifndef SM_H_
#define SM_H_

#include "common.h"

namespace task {

enum class STATE {
    IDLE,
    INITIALIZING,
    RUNNING,
    TERMINATING,
    TERMINATED
};

class  StateMgr
{
public:
    virtual ~StateMgr();

private:


DECLARE_SINGLETON(StateMgr);
};

}

#endif