// synch.cc 
//  Routines for synchronizing threads.  Three kinds of
//  synchronization routines are defined here: semaphores, locks 
//      and condition variables (the implementation of the last two
//  are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
//  Initialize a semaphore, so that it can be used for synchronization.
//
//  "debugName" is an arbitrary name, useful for debugging.
//  "initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
//  De-allocate semaphore, when no longer needed.  Assume no one
//  is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
//  Wait until semaphore value > 0, then decrement.  Checking the
//  value and decrementing must be done atomically, so we
//  need to disable interrupts before checking the value.
//
//  Note that Thread::Sleep assumes that interrupts are disabled
//  when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
      
    //printf("P %s\n", name);
    
    while (value == 0) {            // semaphore not available
    queue->Append((void *)currentThread);   // so go to sleep
        printf("Thread %d Waiting...........%s\n",currentThread->GetThreadID(),  name);
    currentThread->Sleep();
    } 
    value--;                    // semaphore available, 
                        // consume its value
    
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
//  Increment semaphore value, waking up a waiter if necessary.
//  As with P(), this operation must be atomic, so we need to disable
//  interrupts.  Scheduler::ReadyToRun() assumes that threads
//  are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff); 
    //printf("V %s\n", name);
    thread = (Thread *)queue->Remove();
    if (thread != NULL)    // make thread ready, consuming the V immediately
    {
    scheduler->ReadyToRun(thread);
        printf("Thread %d Wakeup............%s\n",currentThread->GetThreadID(),  name);
    }
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) 
{
     name =  debugName;
     value = 1;          // 初值为1
     queue = new List; 
     lockThread = NULL;
}

Lock::~Lock() 
{
     delete queue;
}

void Lock::Acquire() 
{
     IntStatus oldLevel = interrupt->SetLevel(IntOff);
     // 类似于P操作
     while (value == 0)
     {
         queue->Append((void *)currentThread);  
     currentThread->Sleep();
     }
     value--;
     lockThread = currentThread;
     
     (void) interrupt->SetLevel(oldLevel);
}

void Lock::Release() 
{    
     Thread* thread;
     IntStatus oldLevel = interrupt->SetLevel(IntOff);
     
     // 判断当前线程是否持有锁，若没有，则终止
     ASSERT(isHeldByCurrentThread());
     lockThread = NULL;
     thread = (Thread *)queue->Remove();
     if (thread != NULL)
        scheduler->ReadyToRun(thread);
     value++;
   
     (void) interrupt->SetLevel(oldLevel);  
}

bool Lock::isHeldByCurrentThread()
{
     if (lockThread == currentThread)
         return true;
     else 
         return false;
}

// 条件变量
Condition::Condition(char* debugName) 
{ 
     name = debugName;
     queue = new List; 
}

Condition::~Condition()
{
     delete queue;
}


void Condition::Wait(Lock* conditionLock) 
{ 
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      ASSERT(conditionLock->isHeldByCurrentThread());   // 当前线程是否持有该条件（锁）
      conditionLock->Release();
      queue->Append((void *)currentThread);             // 把当前线程放入等待队列，并睡眠
      currentThread->Sleep();
  
      conditionLock->Acquire();                         // 重新获取锁

      (void)interrupt->SetLevel(oldLevel);
      
}

// 唤醒一个线程
void Condition::Signal(Lock* conditionLock)
{ 
      Thread* thread;
      IntStatus oldLevel = interrupt->SetLevel(IntOff);

      ASSERT(conditionLock->isHeldByCurrentThread());   // 当前线程是否持有该条件（锁）
      
      thread = (Thread*)queue->Remove();

      if (thread != NULL)
          scheduler->ReadyToRun(thread);

      (void)interrupt->SetLevel(oldLevel);
  
}
// 唤醒所有线程
void Condition::Broadcast(Lock* conditionLock) 
{ 
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
      ASSERT(conditionLock->isHeldByCurrentThread());   // 当前线程是否持有该条件（锁）
      while (!queue->IsEmpty())                         // 循环唤醒，直到队列为空
          Signal(conditionLock);

      (void)interrupt->SetLevel(oldLevel);
}


Monitor_PC::Monitor_PC(char* debugName)
{
      name = debugName;
      full = new Condition("FULL");
      empty = new Condition("EMPTY");
      CLock = new Lock("ConditionLock");
      count = 0;
      m_in = 0; 
      m_out = 0;
      for (int i = 0; i < MaxBuffer; ++i)
          m_buffer[i] = -1;
}

Monitor_PC::~Monitor_PC()
{
      delete full;
      delete empty;
      delete CLock;
}

void Monitor_PC::insert(int id, char* threadName, int item)
{     
      CLock->Acquire();
      printf("Thread %d Insert..............\n", currentThread->GetThreadID());
      if (count == MaxBuffer)
      {   
          printf("Thread %d Waiting.............%s\n", currentThread->GetThreadID(), full->getName());
          full->Wait(CLock);
      }
      count++;
      m_buffer[m_in] = item;
      interrupt->OneTick();  //模拟时钟移动
      printf("*** thread %d name %s Produce an Item %d, Count: %d\n", id, threadName, item, count);
      interrupt->OneTick();  //模拟时钟移动
      currentThread->Yield();
      m_in = (m_in + 1) % MaxBuffer;
      
      if (count == 1)
      {
          printf("Thread %d Wakeup..............%s\n",currentThread->GetThreadID(),  empty->getName());
          empty->Signal(CLock);
      }
      CLock->Release();  
}

int Monitor_PC::remove(int id, char* threadName)
{
      CLock->Acquire();
      printf("Thread %d Remove..............\n", currentThread->GetThreadID());
      if (count == 0)
      {
          printf("Thread %d Waiting.............%s\n", currentThread->GetThreadID(), empty->getName());
          empty->Wait(CLock);
      }
      count--;
      int val;
      val = m_buffer[m_out];
      m_buffer[m_out] = -1;
      interrupt->OneTick();  //模拟时钟移动
      printf("*** thread %d name %s Consume an Item %d, Count: %d\n", id, threadName, val, count);
      interrupt->OneTick();  //模拟时钟移动
      m_out = (m_out + 1) % MaxBuffer;
      
      
      if (count == MaxBuffer - 1)
      {
          printf("Thread %d Wakeup..............%s\n", currentThread->GetThreadID(), full->getName());
          full->Signal(CLock);
      }
      CLock->Release();
      return val;
}

// Barrier
Barrier::Barrier(char* debugName, int num)
{
      name = debugName;
      waitNum = num;
      ASSERT(waitNum > 0);     // 若waitNum<=0，说明该Barrier无用，终止退出
      status = 0;
      CLock = new Lock("BarrierLock");
      bc = new Condition("BarrierCondition");     
}

Barrier::~Barrier()
{
      delete CLock;
      delete bc;
}

void Barrier::setBarrier()
{
      CLock->Acquire();
      ASSERT(status == 0);     // 试图设置一个已失效的Barrier，则终止
      waitNum--;
      if (waitNum == 0)
      {  
         status = 1;           // 所有线程都到达了，设置该Barrier失效
         bc->Broadcast(CLock); // 将所有等在该Barrier处的线程唤醒
      }
      else
         bc->Wait(CLock);      // 线程数还不够，则等待
      CLock->Release();
}


// ReadWriteLock
ReadWriteLock::ReadWriteLock(char* debugName)
{
      name = debugName;
      mutex = new Lock("Mutex");        // 互斥访问，实现对rCount的互斥修改
      rLock = new Lock("ReadLock");     // 主要用于读写线程的协调
      wLock = new Lock("WriteLock");    // 互斥写
      rCount = 0;                       // 记录正在读的线程数
}

ReadWriteLock::~ReadWriteLock()
{
      delete mutex;
      delete rLock;
      delete wLock;
}

void ReadWriteLock::ReadLockAcquire()
{
      rLock->Acquire();      
      mutex->Acquire();
      rCount++;
      if (rCount == 1)
          wLock->Acquire();  // 第一个读者到来时获取写者的锁
      mutex->Release();
      rLock->Release();
}

void ReadWriteLock::ReadLockRelease()
{
      mutex->Acquire();
      rCount--;
      if (rCount == 0)
          wLock->Release();
      mutex->Release();
}

void ReadWriteLock::WriteLockAcquire()
{
      rLock->Acquire();       // 若在读持续到来中，有写线程到来，则阻止后来的读，防止写线程饥饿
      wLock->Acquire();
      rLock->Release();
}

void ReadWriteLock::WriteLockRelease()
{
      wLock->Release();
}