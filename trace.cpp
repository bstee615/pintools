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
ofstream outFile;


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
		outFile << str;
        outFile << std::flush;
    }
    else
    {
		cout << str;
        cout << std::flush;
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
    // if (!filename.empty()) {
        std::ostringstream ss;
        ss << filename << ":" << line;
        if (KnobReportColumns.Value()) {
            ss << ":" << column;
        }
        ss << " " << asmOrFuncName;
        ss << endl;
        auto location = ss.str();
        write(location);

        // Don't print duplicate lines.
        // if (location != lastLocation) {
        //     write(location);
        //     lastLocation = location;
        // }
    // }
}

// Called on every instruction in a trace.
VOID OnInstruction(INS ins, VOID *v)
{
    OutputSourceLocation(INS_Address(ins), ins); // Calls PIN_GetSourceLocation for a single instruction.
}

// Adding fini function just to close the output file
VOID Fini(INT32 code, VOID *v)
{
    if (!KnobOutputFile.Value().empty())
    {
        outFile.close();
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
		outFile.open(KnobOutputFile.Value().c_str(), std::ofstream::out);
        if(!outFile)
        {
            cerr << "File could not be opened: \"" << KnobOutputFile.Value() << "\"" << endl;
            return -1;
        } 
    }

    // Called on each instruction in the trace
    INS_AddInstrumentFunction(OnInstruction, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();

    return 0;
}
