/*
 * Copyright 2002-2020 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/**
 * This tool prints out each statement in a trace of the program.
 * Taken from the Pintool example code, pin-3.16/source/tools/SimpleExamples/get_source_location.cpp.
 * The old description is included below.
 * 
 * ======================================================================================================
 * 
 * This tool demonstrates the usage of the PIN_GetSourceLocation API from an instrumentation routine. You
 * may notice that there are no analysis routines in this example.
 *
 * Note: According to the Pin User Guide, calling PIN_GetSourceLocation from an analysis routine requires
 *       that the client lock be taken first.
 *
 */

#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "pin.H"
using std::string;

using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::ofstream;

// using namespace CONTROLLER;
// using namespace INSTLIB;

/* ================================================================== */
// Global variables 
/* ================================================================== */

string lastLocation;
ofstream out;


/* ===================================================================== */
// Command line switches
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool",
    "o", "", "specify file name for the tool's output. If no filename is specified, the output will be directed to stdout.");
KNOB<BOOL> KnobReportColumns(KNOB_MODE_WRITEONCE,  "pintool",
    "c", "0", "report column numbers for each location.");


/* ===================================================================== */
// Utilities
/* ===================================================================== */

// Print out help message.
INT32 Usage() {
    cerr << "This tool traces the execution of a command." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

void write(std::string &str)
{
    if (!KnobOutputFile.Value().empty())
    {
		out << str << endl;
    }
    else
    {
		cout << str << endl;
    }
}

// Printing the source location of an instruction.
static void OutputSourceLocation(ADDRINT address, INS ins = INS_Invalid()) {
    string filename;    // This will hold the source file name.
    INT32 line;         // This will hold the line number within the file.
    INT32 column;       // This will hold the column number within the file.

    // Acquiring the client lock is not required in
    // instrumentation functions, only in analysis functions.
    //
    PIN_GetSourceLocation(address, &column, &line, &filename);

    // Prepare the output strings.
    string asmOrFuncName;
    if (INS_Valid(ins)) {
        asmOrFuncName = INS_Disassemble(ins); // For an instruction, get the disassembly.
    }
    else {
        asmOrFuncName = RTN_FindNameByAddress(address); // For a routine, get its name.
    }

    // Print lines only if source was found.
    if (!filename.empty()) {
        std::ostringstream ss;
        ss << filename << ":" << line;
        if (KnobReportColumns.Value()) {
            ss << ":" << column;
        }
        ss << ":" << asmOrFuncName;
        ss << endl;
        auto location = ss.str();

        // Don't print duplicate lines.
        // if (location != lastLocation) {
            write(location);
        //     lastLocation = location;
        // }
    }
}

// Called on every instruction in a trace.
VOID OnInstruction(INS ins, VOID *v)
{
    OutputSourceLocation(INS_Address(ins), ins); // Calls PIN_GetSourceLocation for a single instruction.
}

string FormatAddress(ADDRINT address, RTN rtn)
{
    string s;
    // if (KnobLines)
    // {
        INT32 column;
        INT32 line;
        string file;

        PIN_GetSourceLocation(address, &column, &line, &file);

        if (file != "")
        {
            s += file + ":" + decstr(line) + ":" + decstr(column);
        }
    // }
    return s;
}



// LOCALFUN VOID Flush()
// {
//     out << std::flush;
// }

// LOCALFUN BOOL Emit(THREADID threadid)
// {
//     return true;
// }


/* ===================================================================== */

VOID EmitNoValues(THREADID threadid, string * str)
{
    // if (!Emit(threadid))
    //     return;

    out
        << *str
        << endl;

    //Flush();
}

VOID Emit1Values(THREADID threadid, string * str, string * reg1str, ADDRINT reg1val)
{
    // if (!Emit(threadid))
    //     return;

    out
        << *str //<< " | "
        // << *reg1str << " = " << reg1val
        << endl;

    //Flush();
}

VOID Emit2Values(THREADID threadid, string * str, string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val)
{
    // if (!Emit(threadid))
    //     return;

    out
        << *str //<< " | "
        // << *reg1str << " = " << reg1val
        // << ", " << *reg2str << " = " << reg2val
        << endl;

    //Flush();
}

VOID Emit3Values(THREADID threadid, string * str, string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val, string * reg3str, ADDRINT reg3val)
{
    // if (!Emit(threadid))
    //     return;

    out
        << *str //<< " | "
        // << *reg1str << " = " << reg1val
        // << ", " << *reg2str << " = " << reg2val
        // << ", " << *reg3str << " = " << reg3val
        << endl;

    //Flush();
}


VOID Emit4Values(THREADID threadid, string * str, string * reg1str, ADDRINT reg1val, string * reg2str, ADDRINT reg2val, string * reg3str, ADDRINT reg3val, string * reg4str, ADDRINT reg4val)
{
    // if (!Emit(threadid))
    //     return;

    out
        << *str //<< " | "
        // << *reg1str << " = " << reg1val
        // << ", " << *reg2str << " = " << reg2val
        // << ", " << *reg3str << " = " << reg3val
        // << ", " << *reg4str << " = " << reg4val
        << endl;

    //Flush();
}


const UINT32 MaxEmitArgs = 4;

AFUNPTR emitFuns[] =
{
    AFUNPTR(EmitNoValues), AFUNPTR(Emit1Values), AFUNPTR(Emit2Values), AFUNPTR(Emit3Values), AFUNPTR(Emit4Values)
};

VOID AddEmit(INS ins, IPOINT point, string & traceString, UINT32 regCount, REG regs[])
{
    if (regCount > MaxEmitArgs)
        regCount = MaxEmitArgs;

    IARGLIST args = IARGLIST_Alloc();
    for (UINT32 i = 0; i < regCount; i++)
    {
        IARGLIST_AddArguments(args, IARG_PTR, new string(REG_StringShort(regs[i])), IARG_REG_VALUE, regs[i], IARG_END);
    }

    INS_InsertCall(ins, point, emitFuns[regCount], IARG_THREAD_ID,
                   IARG_PTR, new string(traceString),
                   IARG_IARGLIST, args,
                   IARG_END);
    IARGLIST_Free(args);
}


VOID InstructionTrace(TRACE trace, INS ins)
{
    ADDRINT addr = INS_Address(ins);
    ASSERTX(addr);

    // Format the string at instrumentation time
    string traceString;
    string astring = FormatAddress(INS_Address(ins), TRACE_Rtn(trace));

    if (astring.empty())
        return;
    traceString += astring;

    INT32 regCount = 0;
    REG regs[20];
    // REG xmm_dst = REG_INVALID();

    for (UINT32 i = 0; i < INS_MaxNumWRegs(ins); i++)
    {
        REG x = REG_FullRegName(INS_RegW(ins, i));

        if (REG_is_gr(x)
#if defined(TARGET_IA32)
            || x == REG_EFLAGS
#elif defined(TARGET_IA32E)
            || x == REG_RFLAGS
#endif
        )
        {
            regs[regCount] = x;
            regCount++;
        }

        // if (REG_is_xmm(x))
        //     xmm_dst = x;
    }

    if (INS_IsValidForIpointAfter(ins))
    {
        AddEmit(ins, IPOINT_AFTER, traceString, regCount, regs);
    }
    if (INS_IsValidForIpointTakenBranch(ins))
    {
        AddEmit(ins, IPOINT_TAKEN_BRANCH, traceString, regCount, regs);
    }

    // if (xmm_dst != REG_INVALID())
    // {
    //     if (INS_IsValidForIpointAfter(ins))
    //         AddXMMEmit(ins, IPOINT_AFTER, xmm_dst);
    //     if (INS_IsValidForIpointTakenBranch(ins))
    //         AddXMMEmit(ins, IPOINT_TAKEN_BRANCH, xmm_dst);
    // }
}

VOID Trace(TRACE trace, VOID* v)
{
    // Check that the instruction is in the executable.
    // This excludes a lot of library and preamble/postamble code.
    RTN rtn = TRACE_Rtn(trace);
    if (RTN_Valid(rtn) && !IMG_IsMainExecutable(SEC_Img(RTN_Sec(rtn))))
    {
        return;
    }

    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            // OnInstruction(ins, 0);
            InstructionTrace(trace, ins);

            // CallTrace(trace, ins);

            // MemoryTrace(ins);
        }
    }
}


// Adding fini function just to close the output file
VOID Fini(INT32 code, VOID *v)
{
    if (!KnobOutputFile.Value().empty())
    {
        out.close();
    }
}

/* ===================================================================== */
// main
/* ===================================================================== */

int main(INT32 argc, CHAR **argv) {
    PIN_InitSymbols();

    if(PIN_Init(argc,argv)) {
        return Usage();
    }

    if (!KnobOutputFile.Value().empty())
    {
		out.open(KnobOutputFile.Value().c_str(), std::ofstream::out);
        if(!out)
        {
            cerr << "File could not be opened: \"" << KnobOutputFile.Value() << "\"" << endl;
            return -1;
        } 
    }

    // Called on each instruction in the trace
    // INS_AddInstrumentFunction(OnInstruction, 0);
    TRACE_AddInstrumentFunction(Trace, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
