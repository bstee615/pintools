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

#include <iostream>
#include <fstream>
#include "pin.H"
using std::string;

using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::ofstream;


/* ================================================================== */
// Global variables 
/* ================================================================== */

string lastLocation;


/* ===================================================================== */
// Command line switches
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool",
    "o", "", "specify file name for the tool's output. If no filename is specified, the output will be directed to stdout.");


/* ===================================================================== */
// Utilities
/* ===================================================================== */

// Print out help message.
INT32 Usage() {
    cerr << "This tool demonstrates the usage of the PIN_GetSourceLocation API." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

// Printing the source location of an instruction.
static void OutputSourceLocation(ADDRINT address, ostream* printTo, INS ins = INS_Invalid()) {
    string filename;    // This will hold the source file name.
    INT32 line = 0;     // This will hold the line number within the file.

    // In this example, we don't print the column number so there is no reason to obtain it.
    // Simply pass a NULL pointer instead. Also, acquiring the client lock is not required in
    // instrumentation functions, only in analysis functions.
    //
    PIN_GetSourceLocation(address, NULL, &line, &filename);

    // Prepare the output strings.
    string asmOrFuncName;
    if (INS_Valid(ins)) {
        asmOrFuncName = INS_Disassemble(ins); // For an instruction, get the disassembly.
    }
    else {
        asmOrFuncName = RTN_FindNameByAddress(address); // For a routine, get its name.
    }
    // *printTo << asmOrFuncName << endl;

    // Print lines only if source was found.
    if (!filename.empty()) {
        std::ostringstream ss;
        ss << filename << ":" << line << endl;
        auto location = ss.str();

        // Don't print duplicate lines.
        if (location != lastLocation) {
            *printTo << location;
            lastLocation = location;
        }
    }
}

// Called on every instruction in a trace.
VOID OnInstruction(INS ins, VOID *v)
{
    OutputSourceLocation(INS_Address(ins), static_cast<ostream*>(v), ins); // Calls PIN_GetSourceLocation for a single instruction.
}

// Adding fini function just to close the output file
VOID Fini(INT32 code, VOID *v)
{
    if (!KnobOutputFile.Value().empty() && v != NULL)
    {
		static_cast<ofstream*>(v)->close();
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

    ofstream outFile;
    if (!KnobOutputFile.Value().empty())
    {
		outFile.open(KnobOutputFile.Value().c_str());
    }

    INS_AddInstrumentFunction(OnInstruction, (KnobOutputFile.Value().empty()) ? &cout : &outFile);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, &outFile);

    // Never returns
    PIN_StartProgram();

    return 0;
}
