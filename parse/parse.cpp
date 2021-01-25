#include <clang-c/Index.h>
#include <iostream>
#include <string>

// https://bastian.rieck.ru/blog/posts/2015/baby_steps_libclang_ast/

std::string getCursorKindName(CXCursorKind cursorKind)
{
    CXString kindName = clang_getCursorKindSpelling(cursorKind);
    std::string result = clang_getCString(kindName);

    clang_disposeString(kindName);
    return result;
}

std::string getCursorSpelling(CXCursor cursor)
{
    CXString cursorSpelling = clang_getCursorSpelling(cursor);
    std::string result = clang_getCString(cursorSpelling);

    clang_disposeString(cursorSpelling);
    return result;
}

/// Convert CXClientData to uint.
unsigned int as_uint(CXClientData clientData)
{
    return *(reinterpret_cast<unsigned int *>(clientData));
}

CXChildVisitResult visitor(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
    CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isFromMainFile(location) == 0)
        return CXChildVisit_Continue;

    CXCursorKind cursorKind = clang_getCursorKind(cursor);

    unsigned int curLevel = as_uint(clientData);
    unsigned int nextLevel = curLevel + 1;

    std::cout << std::string(curLevel, '-') << " " << getCursorKindName(cursorKind) << " (" << getCursorSpelling(cursor) << ")\n";

    clang_visitChildren(cursor,
                        visitor,
                        &nextLevel);

    return CXChildVisit_Continue;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file.c>" << std::endl;
        return -1;
    }

    const char *const *clang_command_line_args = nullptr;
    struct CXUnsavedFile *unsaved_files = nullptr;

    CXIndex index = clang_createIndex(0, 1);
    CXTranslationUnit tu = clang_parseTranslationUnit(index, argv[1], clang_command_line_args, 0, unsaved_files, 0, 0);

    if (!tu) {
        std::cerr << "Could not create translation unit from " << argv[1] << std::endl;
        return -1;
    }

    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);

    unsigned int treeLevel = 0;

    clang_visitChildren(rootCursor, visitor, &treeLevel);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return 0;
}