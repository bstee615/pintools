#include "parse.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <source_file.c>" << std::endl;
        return -1;
    }

    auto tags = parse(argv[1], {CXCursor_VarDecl, CXCursor_CaseStmt, CXCursor_DefaultStmt});
    for (auto l : tags)
    {
        std::cout << l.filename << ":" << l.lineno << std::endl;
    }
}
