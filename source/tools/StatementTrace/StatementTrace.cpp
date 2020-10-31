
/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
using std::cerr;
using std::string;
using std::endl;

std::ostream * out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool",
    "o", "", "specify file name for StatementTrace output");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool prints out a dynamic trace at every statement in the application ";

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

struct Location {
    INT32 column;
    INT32 line;
    string filename;
};
std::vector<Location> locations;

/*!
 * Log every step of the trace.
 * 
 * This function is called every time a new trace is encountered.
 * @param[in]   ins      instruction to be instrumented
 * @param[in]   v        value specified by the tool in the TRACE_AddInstrumentFunction
 *                       function call
 */
VOID OnInstruction(INS ins, VOID *v)
{
    Location l;
    LEVEL_PINCLIENT::PIN_GetSourceLocation(INS_Address(ins), &l.column, &l.line, &l.filename);
    // *out << l.filename << ":" << l.line << endl;
    if (!l.filename.empty()) {
        locations.push_back(l);
    }
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v)
{
    *out <<  "===============================================" << endl;
    *out <<  "StatementTrace analysis results: " << endl;
    for (auto i = locations.begin(); i != locations.end(); i ++) {
        *out << "Location: " << (*i).filename << ":" << (*i).line << endl;
    }
    *out <<  "===============================================" << endl;
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid 
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    PIN_InitSymbols();
    
    string fileName = KnobOutputFile.Value();

    if (!fileName.empty()) { out = new std::ofstream(fileName.c_str());}

    // Register function to be called to instrument traces
    INS_AddInstrumentFunction(OnInstruction, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    cerr <<  "===============================================" << endl;
    cerr <<  "This application is instrumented by StatementTrace" << endl;
    if (!KnobOutputFile.Value().empty()) 
    {
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    }
    cerr <<  "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
