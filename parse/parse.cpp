#include <clang-c/Index.h>
#include <iostream>
#include <string>

// https://bastian.rieck.ru/blog/posts/2015/baby_steps_libclang_ast/

/// The information we need at one level of the traversal.
struct WalkParams {
    unsigned int level;
    CXCursorKind seeking;

    WalkParams(CXCursorKind _seeking) {
        level = 0;
        seeking = _seeking;
    }

    WalkParams(const WalkParams *other) {
        level = other->level;
        seeking = other->seeking;
    }

    static WalkParams *get(CXClientData data) {
        return reinterpret_cast<WalkParams *>(data);
    }
};

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

CXChildVisitResult visitor(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
    // Exclude locations not in our main file.
    // TODO: Filter to source files we have traced with Pin.
    CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isFromMainFile(location) == 0)
        return CXChildVisit_Continue;

    // Get data into workable form
    auto params = WalkParams::get(clientData);
    CXCursorKind cursorKind = clang_getCursorKind(cursor);

    // If this is the level we want, tag it
    bool tagged = false;
    if (cursorKind == params->seeking) {
        tagged = true;
    }

    // Log level
    std::cout << std::string(params->level, '-') << " " << getCursorKindName(cursorKind) << " (" << getCursorSpelling(cursor) << ")" << (tagged ? " *" : "") << "\n";

    // Create next call's params and recurse
    unsigned int nextLevel = params->level + 1;
    WalkParams nextParams = WalkParams(params);
    nextParams.level = nextLevel;
    clang_visitChildren(cursor,
                        visitor,
                        &nextParams);

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

    WalkParams params(CXCursor_VarDecl);
    clang_visitChildren(rootCursor, visitor, &params);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return 0;
}