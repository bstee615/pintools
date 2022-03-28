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
KNOB<BOOL> KnobReportMainOnly(KNOB_MODE_WRITEONCE,  "pintool",
    "m", "0", "report lines from main executable only.");


/* ===================================================================== */
// Utilities
/* ===================================================================== */

// Print out help message.
INT32 Usage() {
    cerr << "This tool traces the execution of a command." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

string FormatAddress(ADDRINT address, RTN rtn)
{
    INT32 column;
    INT32 line;
    string file;

    PIN_GetSourceLocation(address, &column, &line, &file);

    if (file != "")
    {
        file += ":" + decstr(line);
        if (KnobReportColumns.Value()) {
            file += ":" + decstr(column);
        }
    }
    return file;
}


VOID Emit(string * str)
{
    out << *str << endl;
}


AFUNPTR emitFn = AFUNPTR(Emit);


VOID AddEmit(INS ins, IPOINT point, string &traceString)
{
    INS_InsertCall(ins, point, emitFn, IARG_PTR, new string(traceString), IARG_END);
}


VOID InstructionTrace(TRACE trace, INS ins)
{
    ADDRINT addr = INS_Address(ins);
    ASSERTX(addr);

    // Format the string at instrumentation time
    string addressString = FormatAddress(INS_Address(ins), TRACE_Rtn(trace));
    if (addressString.empty())
        return;

    if (INS_IsValidForIpointAfter(ins))
    {
        AddEmit(ins, IPOINT_AFTER, addressString);
    }
    if (INS_IsValidForIpointTakenBranch(ins))
    {
        AddEmit(ins, IPOINT_TAKEN_BRANCH, addressString);
    }
}

VOID Trace(TRACE trace, VOID* v)
{
    // Check that the instruction is in the executable.
    // This excludes a lot of library and preamble/postamble code.
    RTN rtn = TRACE_Rtn(trace);
    if (!RTN_Valid(rtn))
    {
        return;
    }
    else if (KnobReportMainOnly.Value() && !IMG_IsMainExecutable(SEC_Img(RTN_Sec(rtn))))
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
