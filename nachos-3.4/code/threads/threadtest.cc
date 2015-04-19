// threadtest.cc 
//  Simple test case for the threads assignment.
//
//  Create two threads, and have them context switch
//  back and forth between themselves by calling Thread::Yield, 
//  to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// testnum is set in main.cc
int testnum = 1;

char* threadStatusStr[4] = {"JUST_CREATED", "RUNNING", "READY", "BLOCKED"};  
//----------------------------------------------------------------------
// SimpleThread
//  Loop 5 times, yielding the CPU to another ready thread 
//  each iteration.
//
//  "which" is simply a number identifying the thread, for debugging
//  purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 6; num++) {
    printf("*** thread  %d name:\"%s\" looped %d times\n", which, currentThread->getName(),  num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
//  Set up a ping-pong between two threads, by forking a thread 
//  to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------


void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    //Thread *t = new Thread("forked thread");
    Thread *t1 = new Thread("thread1");
    Thread *t2 = new Thread("thread2");
    Thread *t3 = new Thread("thread3");
       // t1->setPriority(120);
       // t2->setPriority(80);
       // t3->setPriority(95);
    //t->Fork(SimpleThread, 1);
    // t1->Fork(SimpleThread, t);
    t1->Fork(SimpleThread, t1->GetThreadID());
    t2->Fork(SimpleThread, t2->GetThreadID());
    t3->Fork(SimpleThread, t3->GetThreadID());

//    SimpleThread(0);

}


void 
ShowThreads()
{
    printf(" TID  UID  NAME    STATUS   \n");  
    for (int i = 0; i < MaxThreadNum; ++i)
    {
        if (threadsInfo[i] != NULL)
        {
            Thread* temp = threadsInfo[i];
            printf("  %d    %d   %s    %s   \n", temp->GetThreadID(), temp->GetUserID(), temp->getName(), threadStatusStr[temp->getStatus()]);
        }
    }
}

const int MaxOps = 5;
const int BufferNum = 5;
int buffer[BufferNum] = {-1, -1, -1, -1, -1};
int in = 0;
int out = 0;
int full_count = 0;
int empty_count = 5;

void Producer(int which)
{
    int i;
    for (i = 0; i < MaxOps; ++i)
    {
         if (empty_count > 0)
         {
             full_count++;
             empty_count--;
             buffer[in] = i+which*MaxOps;
             interrupt->OneTick();  //模拟时钟移动
             printf("*** thread %d Produce an Item %d, Full: %d, Empty: %d\n", which, i+which*MaxOps, full_count, empty_count);
             interrupt->OneTick();  //模拟时钟移动
             currentThread->Yield(); // 为了促成可能的错误
             in = (in + 1) % BufferNum;
             if (i % 2 == 1)
                currentThread->Yield();
         }
         else
         {
           i--;
           interrupt->OneTick();  //模拟时钟移动
         }
    }
}

void Consumer(int which)
{ 
    int i;
    for (i = 0; i < MaxOps*2; ++i)
    {
        if (full_count > 0)
        {
            full_count--;
            empty_count++;
            int val = buffer[out];
            buffer[out] = -1;
            interrupt->OneTick();  //模拟时钟移动
            printf("*** thread %d Consume an Item %d, Full: %d, Empty: %d\n", which, val, full_count, empty_count);
            interrupt->OneTick();  //模拟时钟移动
            out = (out + 1) % BufferNum;
        }
        else 
        {
            i--;
            interrupt->OneTick();  //模拟时钟移动
        }
    }
}

void NoSynchTest()
{
    DEBUG('t', "Entering NoSynchTest");

    Thread* pt1 = new Thread("Producer1");
    Thread* pt2 = new Thread("Producer2");
    Thread* ct = new Thread("Consumer");

    pt1->Fork(Producer, pt1->GetThreadID());
    pt2->Fork(Producer, pt2->GetThreadID());
    ct->Fork(Consumer, ct->GetThreadID());
   
}
// 信号量实现生产者消费者问题
Semaphore *full = new Semaphore("FULL", 0);
Semaphore *empty = new Semaphore("EMPTY", 5);
Semaphore *mutex = new Semaphore("MUTEX", 1);

void ProducerTest(int which)
{
    int i;
    for (i = 0; i < MaxOps; ++i)
    { 
        empty->P();
        mutex->P();
        full_count++;
        empty_count--;
        buffer[in] = i+which*MaxOps;
        interrupt->OneTick();  //模拟时钟移动
        printf("*** thread %d Produce an Item %d, Full: %d, Empty: %d\n", which, i+which*MaxOps, full_count, empty_count);
        interrupt->OneTick();  //模拟时钟移动
        currentThread->Yield();  // 为了促成可能的错误
        in = (in + 1) % BufferNum;
        mutex->V();
        full->V();
        if (i % 2 == 1)
            currentThread->Yield();
    }
}

void ConsumerTest(int which)
{
    int i;
    for (i = 0; i < MaxOps*2; ++i)
    {
        full->P();
        mutex->P();
        full_count--;
        empty_count++;
        int val = buffer[out];
        buffer[out] = -1;
        interrupt->OneTick();  //模拟时钟移动
        printf("*** thread %d Consume an Item %d, Full: %d, Empty: %d\n", which, val, full_count, empty_count);
        interrupt->OneTick();  //模拟时钟移动
        out = (out + 1) % BufferNum;
        mutex->V();
        empty->V();
    }
}

     
void TestSemaphore()
{
    DEBUG('t', "Entering TestSemaphore");

    Thread* pt1 = new Thread("Producer1");
    Thread* pt2 = new Thread("Producer2");
    Thread* ct = new Thread("Consumer");
   

    pt1->Fork(ProducerTest, pt1->GetThreadID());
    pt2->Fork(ProducerTest, pt2->GetThreadID());
    ct->Fork(ConsumerTest, ct->GetThreadID());

}


// 条件变量实现生产者消费者问题
Monitor_PC* TestPC = new Monitor_PC("ProducerConsumer");


void MonitorProducer(int which)
{
    int i;
    for (i = 0; i < MaxOps; ++i)
    {
        //printf("Producer\n");
        TestPC->insert(which, currentThread->getName(), i+which*MaxOps);
        if (i % 2 == 1)
            currentThread->Yield();
    }
}

void MonitorConsumer(int which)
{
    int i;
    for (i = 0; i < MaxOps*2; ++i)
    { 
        int val;
        //printf("Consumer\n");
        val = TestPC->remove(which, currentThread->getName());
    }
}

void TestCondition()
{
    DEBUG('t', "Entering TestCondition");
    
    Thread* pt1 = new Thread("Producer1");
    Thread* pt2 = new Thread("Producer2");
    Thread* ct = new Thread("Consumer");

    pt1->Fork(MonitorProducer, pt1->GetThreadID());
    pt2->Fork(MonitorProducer, pt2->GetThreadID());
    ct->Fork(MonitorConsumer, ct->GetThreadID());

}

Barrier* barrier = new Barrier("BarrierTest", 5);

void BarrierSetTest(int which)
{
    interrupt->OneTick();
    printf("Thread %d Name %s Waiting Here!\n", which, currentThread->getName());
    barrier->setBarrier();
    printf("Thread %d Name %s Continue to Run\n", which, currentThread->getName());
    interrupt->OneTick();
    interrupt->OneTick();   
}
void BarrierTest()
{
    DEBUG('t', "Entering BarrierTest");
    Thread* t1 = new Thread("Barrier1");
    Thread* t2 = new Thread("Barrier2");
    Thread* t3 = new Thread("Barrier3");
    Thread* t4 = new Thread("Barrier4");
    Thread* t5 = new Thread("Barrier5");

    t1->Fork(BarrierSetTest, t1->GetThreadID());
    t2->Fork(BarrierSetTest, t2->GetThreadID());
    t3->Fork(BarrierSetTest, t3->GetThreadID());
    t4->Fork(BarrierSetTest, t4->GetThreadID());
    t5->Fork(BarrierSetTest, t5->GetThreadID());
}


int shared[3] = {100, 100, 100};
ReadWriteLock* rwLock = new ReadWriteLock("ReadWriteLock");

void ReadTest(int which)
{ 
    rwLock->ReadLockAcquire();
    int i;
    for (i = 0; i < 3; ++i)
    {
        printf("*** Thread %d Name %s Read Shared Array %d: %d\n", which, currentThread->getName(), i, shared[i]);
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
    }
    rwLock->ReadLockRelease();
}

void WriteTest(int which)
{
    rwLock->WriteLockAcquire();
    int i;
    for (i = 0; i < 3; ++i)
    {
        shared[i] -= 1;
        printf("*** Thread %d Name %s Write Shared Array %d: %d\n", which, currentThread->getName(), i, shared[i]);
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
        interrupt->OneTick();
    }
    rwLock->WriteLockRelease();
}
void RWLockTest()
{
    DEBUG('t', "Entering RWLockTest");
    
    Thread* r1 = new Thread("Read1");
    Thread* r2 = new Thread("Read2");
    Thread* w1 = new Thread("Write1");
    Thread* r3 = new Thread("Read3");
    Thread* w2 = new Thread("Write2");
    Thread* r4 = new Thread("Read4");
    Thread* w3 = new Thread("Write3");
    Thread* w4 = new Thread("Write4");
    Thread* r5 = new Thread("Read5");

    r1->Fork(ReadTest, r1->GetThreadID());
    r2->Fork(ReadTest, r2->GetThreadID());
    w1->Fork(WriteTest, w1->GetThreadID());
    r3->Fork(ReadTest, r3->GetThreadID());
    w2->Fork(WriteTest, w2->GetThreadID());
    r4->Fork(ReadTest, r4->GetThreadID());
    w3->Fork(WriteTest, w3->GetThreadID());
    w4->Fork(WriteTest, w4->GetThreadID());
    r5->Fork(ReadTest, r5->GetThreadID());
    
}
//----------------------------------------------------------------------
// ThreadTest
//  Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
       ThreadTest1();
       //TestCondition();
       //TestSemaphore();
       //NoSynchTest();
       //BarrierTest();
       //RWLockTest();
    break;
    case 2:
    ThreadTest1();
    ShowThreads();
    break;
    default:
    printf("No test specified.\n");
    break;
    }
}
