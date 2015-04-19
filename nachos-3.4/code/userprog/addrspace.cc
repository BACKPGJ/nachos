// addrspace.cc 
//  Routines to manage address spaces (executing user programs).
//
//  In order to run a user program, you must:
//
//  1. link with the -N -T 0 option 
//  2. run coff2noff to convert the object file to Nachos format
//      (Nachos object code format is essentially just a simpler
//      version of the UNIX executable object code format)
//  3. load the NOFF file into the Nachos file system
//      (if you haven't implemented the file system yet, you
//      don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// SwapHeader
//  Do little endian to big endian conversion on the bytes in the 
//  object file header, in case the file was generated on a little
//  endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//  Create an address space to run a user program.
//  Load the program from a file "executable", and set everything
//  up so that we can start executing user instructions.
//
//  Assumes that the object code file is in NOFF format.
//
//  First, set up the translation from program memory to physical 
//  memory.  For now, this is really simple (1:1), since we are
//  only uniprogramming, and we have a single unsegmented page table
//
//  "executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    unsigned int i, size;
    NoffHeader noffH;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

   
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;    // we need to increase the size
                        // to leave room for the stack
   printf("NumPages: %d\n", numPages);
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
    printf("NumPages: %d\n", numPages);

  //  ASSERT(numPages <= NumPhysPages);     // check we're not trying
                        // to run anything too big --
                        // at least until we have
                        // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
                    numPages, size);
// first, set up the translation 
 
     pageTable = new TranslationEntry[numPages];
    printf("PageTable Address: 0x%x\n", (unsigned int)pageTable);
    for (i = 0; i < numPages; ++i) {
        pageTable[i].valid = FALSE;
        pageTable[i].dirty = FALSE;
    }
    char* threadName = currentThread->getName();
    char fileName[32];
    strcpy(fileName, threadName);
    char* endFile = ".va";
    strcat(fileName, endFile);
    printf("%s\n", fileName);
    if (fileSystem->Create(fileName, size))
        vaSpace = fileSystem->Open(fileName);
    else {
        printf("Can not Create the File!\n");
        ASSERT(FALSE);
    }
  
    printf("NumPages: %d\n", numPages);
    printf("NoffHeader: \n");
    printf("Code: 0x%x, 0x%x, 0x%x\n", noffH.code.virtualAddr, noffH.code.inFileAddr, noffH.code.size);
    printf("Data:  0x%x, 0x%x, 0x%x\n", noffH.initData.virtualAddr, noffH.initData.inFileAddr, noffH.initData.size);
    printf("UninitDate: 0x%x,  0x%x,  0x%x\n", noffH.uninitData.virtualAddr, noffH.uninitData.inFileAddr, noffH.uninitData.size);
// then, copy in the code and data segments into virtualspace

    char* tempContainer = new char[size];
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
            noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(tempContainer[0]), noffH.code.size, noffH.code.inFileAddr);
        vaSpace->WriteAt(&(tempContainer[0]), noffH.code.size, 0);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
            noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(tempContainer[noffH.code.size]), noffH.initData.size, noffH.initData.inFileAddr);
        vaSpace->WriteAt(&(tempContainer[noffH.code.size]), noffH.initData.size, noffH.code.size);
    }
    delete tempContainer;
    
    int codePages = divRoundUp(noffH.code.size, PageSize);
    for (i = 0; i < codePages; ++i) {
        int ppn = machine->mBitMap->Find();
        if (ppn == -1) {
            printf("The physical memory is Full!\n");
            break;
        }
        printf("PPN %d\n", ppn);
        pageTable[i].physicalPage = ppn;
        pageTable[i].virtualPage = i;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;
        vaSpace->ReadAt(&(machine->mainMemory[ppn*PageSize]), PageSize, i*PageSize);
        machine->physPageTable[ppn].vaPageNum = i;
        machine->physPageTable[ppn].valid = TRUE;
        machine->physPageTable[ppn].dirty = FALSE;
        machine->physPageTable[ppn].heldThreadID = currentThread->GetThreadID();
    }
    
   
    printf("hahhahaha\n");
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//  Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
   delete vaSpace;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//  Set the initial values for the user-level register set.
//
//  We write these directly into the "machine" registers, so
//  that we can immediately jump to user code.  Note that these
//  will be saved/restored into the currentThread->userRegisters
//  when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);   

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//  On a context switch, save any machine state, specific
//  to this address space, that needs saving.
//
//  For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    machine->ClearTLB();
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//  On a context switch, restore the machine state so that
//  this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}