#ifndef PARSE_H
#define PARSE_H

#include <clang-c/Index.h>
#include <vector>
#include <string>

struct Location
{
    std::string filename;
    unsigned int lineno;

    Location(CXCursor cursor);
};

std::vector<Location> parse(const char *source_filename, std::vector<CXCursorKind> seeking);

#endif // PARSE_H
