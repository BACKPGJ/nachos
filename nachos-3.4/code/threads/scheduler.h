// scheduler.h 
//  Data structures for the thread dispatcher and scheduler.
//  Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

//#define TimerTicks 200  // 时间片大小，TimeTicks在stats.h中已有定义
#define CreatePriority 100      // 创建时的优先级
#define BlockedPriority 70      // 从Sleep唤醒时的优先级 
#define AdaptPace -5            // 调整幅度
#define MinTicks (TimerTicks/8) // 最小时间片

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();      // Initialize list of ready threads 
    ~Scheduler();     // De-allocate ready list

    void ReadyToRun(Thread* thread);  // Thread can be dispatched.
    Thread* FindNextToRun(bool fromSleep);    // Dequeue first thread on the ready   //fromSleep用于判断是否来自sleep，如果是的，进行不一样的操作
          // list, if any, and return thread.
    void Run(Thread* nextThread); // Cause nextThread to start running
    void Print();     // Print contents of ready list
    
    void FlushPriority();             // 时钟中断发生时，对所有Ready线程进行优先级调整，（+AdaptPace）
    int getLastSwitchTicks() { return lastSwitchTicks; }
    
  private:
    List *readyList;      // queue of threads that are ready to run,
        // but not running
    int lastSwitchTicks;
};

#endif // SCHEDULER_H