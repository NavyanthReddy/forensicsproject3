/*BEGIN_LEGAL 
		Intel Open Source License 

		Copyright (c) 2002-2016 Intel Corporation. All rights reserved.

		Redistribution and use in source and binary forms, with or without
		modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
	*  This file contains an ISA-portable PIN tool for tracing system calls
	*/


#define NDEBUG

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/syscall.h>
#include "pin.H"

#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <iostream>

using std::string;
using std::cerr;
using std::endl;

FILE * trace;
bool isMainProgram = false;

KNOB<BOOL>   KnobReplay(KNOB_MODE_WRITEONCE,  "pintool",
				"replay", "0", "replay the program");

KNOB<string> KnobLogFile(KNOB_MODE_WRITEONCE, "pintool", "o", "trace.out", "specify trace file name");

static VOID SysBefore(ADDRINT ip, ADDRINT num, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, 
				ADDRINT arg3, ADDRINT arg4, ADDRINT arg5, CONTEXT *ctxt)
{
		if(!isMainProgram) return;

		if(KnobReplay == true)
		{
				//Replay the program
		}
}


VOID SysAfter(ADDRINT ret, ADDRINT err)
{
		if(!isMainProgram) return;
		
		if(KnobReplay == false)
		{
				// Record non-deterministic inputs and store to "trace"
		}
}

VOID MainBegin()
{
						isMainProgram = true;
}

VOID MainReturn()
{
						isMainProgram = false;
}

VOID Image(IMG img, VOID *v)
{
		RTN mainRtn = RTN_FindByName(img, "main");
		if(RTN_Valid(mainRtn))
		{
				RTN_Open(mainRtn);
				RTN_InsertCall(mainRtn, IPOINT_BEFORE, (AFUNPTR)MainBegin, IARG_END);
				RTN_InsertCall(mainRtn, IPOINT_AFTER, (AFUNPTR)MainReturn, IARG_END);
				RTN_Close(mainRtn);
		}
}

VOID Instruction(INS ins, VOID *v)
{
		if(INS_IsSyscall(ins)) {

				// Arguments and syscall number is only available before
				INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SysBefore),
								IARG_INST_PTR, IARG_SYSCALL_NUMBER,
								IARG_SYSARG_VALUE, 0, IARG_SYSARG_VALUE, 1,
								IARG_SYSARG_VALUE, 2, IARG_SYSARG_VALUE, 3,
								IARG_SYSARG_VALUE, 4, IARG_SYSARG_VALUE, 5,
								IARG_CONTEXT, IARG_END);
		}
}


VOID SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
		SysAfter(PIN_GetSyscallReturn(ctxt, std), PIN_GetSyscallErrno(ctxt, std));
}

VOID Fini(INT32 code, VOID *v)
{
		fclose(trace);
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
		cerr <<
				"This tool record/replay the program.\n"
				"\n";

		cerr << KNOB_BASE::StringKnobSummary();

		cerr << endl;

		return -1; 

}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
		PIN_InitSymbols();

		if (PIN_Init(argc, argv)) return Usage();

		if (KnobReplay)
		{
				printf("====== REPLAY MODE =======\n");
				trace = fopen(KnobLogFile.Value().c_str(), "rb");
		}
		else
		{
				printf("====== RECORDING MODE =======\n");
				trace = fopen(KnobLogFile.Value().c_str(), "wb");
		}

		if(trace == NULL)
		{
				fprintf(stderr, "File open error! (trace.out)\n");
				return 0;
		}


		IMG_AddInstrumentFunction(Image, 0);
		INS_AddInstrumentFunction(Instruction, 0);
		PIN_AddSyscallExitFunction(SyscallExit, 0);


		PIN_AddFiniFunction(Fini, 0);

		// Never returns
		PIN_StartProgram();

		return 0;
}
