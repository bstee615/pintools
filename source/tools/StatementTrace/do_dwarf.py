
from elftools.common.py3compat import maxint, bytes2str
from elftools.dwarf.descriptions import describe_form_class
from elftools.elf.elffile import ELFFile



def process_file(filename, file, line):
    print('Processing file:', filename)
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)

        if not elffile.has_dwarf_info():
            raise 'file has no DWARF info'

        # get_dwarf_info returns a DWARFInfo context object, which is the
        # starting point for all DWARF-based processing in pyelftools.
        dwarfinfo = elffile.get_dwarf_info()

        match_file_line(dwarfinfo, file, line)

def match_file_line(dwarfinfo, f, l):
    # Go over all the line programs in the DWARF information, looking for
    # one that describes the given address.
    for CU in dwarfinfo.iter_CUs():
        # First, look at line programs to find the file/line for the address
        lineprog = dwarfinfo.line_program_for_CU(CU)
        for entry in lineprog.get_entries():
            print(entry, entry.state)
            # We're interested in those entries where a new state is assigned
            if entry.state is None:
                continue
            if entry.state.end_sequence:
                # if the line number sequence ends, clear prevstate.
                continue
            state = entry.state
            file = lineprog['file_entry'][state.file-1].name.decode('utf-8')
            line = state.line
            if file == f and line == l:
                print('match', file, line)
                # print(entry)
                # print(state)
                
if __name__ == '__main__':
    process_file('test', 'test.c', 10)
    