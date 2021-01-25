#include <clang-c/Index.h>
#include <iostream>
#include <string>

// https://bastian.rieck.ru/blog/posts/2015/baby_steps_libclang_ast/

/// The information we need at one level of the traversal.
struct WalkParams
{
private:

    // Get params from CXClientData during the traversal.
    WalkParams(CXCursorKind seeking) : level(0), seeking(seeking) {}

public:
    unsigned int level;
    CXCursorKind seeking;
    CXCursor cursor;

    // Copy params from CXCursor and CXClientData during the traversal.
    WalkParams(CXCursor cursor, CXClientData data) : WalkParams(*reinterpret_cast<WalkParams *>(data))
    {
        this->cursor = cursor;
    }

    WalkParams(const WalkParams &currentParams) : level(currentParams.level),
                                                  seeking(currentParams.seeking),
                                                  cursor(currentParams.cursor) {}

    static WalkParams initial(CXCursorKind seeking)
    {
        return WalkParams(seeking);
    }

    static WalkParams next(const WalkParams &currentParams)
    {
        WalkParams next(currentParams);
        next.level++;
        return next;
    }

    // Exclude locations not in our main file.
    // TODO: Filter to source files we have traced with Pin.
    bool isValidLocation()
    {
        CXSourceLocation location = clang_getCursorLocation(cursor);
        if (clang_Location_isFromMainFile(location) == 0)
            return CXChildVisit_Continue;
    }

    // Getter for the current CXCursorKind.
    CXCursorKind cursorKind()
    {
        return clang_getCursorKind(cursor);
    }

    // Return whether the cursor matches the kind we are seeking.
    bool cursorMatches()
    {
        return cursorKind() == seeking;
    }

    // Return string representation of current CXCursorKind.
    std::string getCursorKindName()
    {
        CXString kindName = clang_getCursorKindSpelling(cursorKind());
        std::string result = clang_getCString(kindName);

        clang_disposeString(kindName);
        return result;
    }

    // Return string representation of current CXCursor's spelling.
    std::string getCursorSpelling()
    {
        CXString cursorSpelling = clang_getCursorSpelling(cursor);
        std::string result = clang_getCString(cursorSpelling);

        clang_disposeString(cursorSpelling);
        return result;
    }
};

CXChildVisitResult visitor(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
{
    WalkParams currentParams(cursor, clientData);

    // Get data into workable form
    CXCursorKind cursorKind = clang_getCursorKind(cursor);

    // If this is the level we want, tag it and log
    bool tagged = currentParams.cursorMatches();
    std::cout << std::string(currentParams.level, '-') << " " << currentParams.getCursorKindName() << " (" << currentParams.getCursorSpelling() << ")" << (tagged ? " *" : "") << "\n";

    // Create next call's params and recurse
    WalkParams nextParams = WalkParams::next(currentParams);
    clang_visitChildren(cursor,
                        visitor,
                        &nextParams);
    return CXChildVisit_Continue;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <source_file.c>" << std::endl;
        return -1;
    }

    const char *const *clang_command_line_args = nullptr;
    struct CXUnsavedFile *unsaved_files = nullptr;

    CXIndex index = clang_createIndex(0, 1);
    CXTranslationUnit tu = clang_parseTranslationUnit(index, argv[1], clang_command_line_args, 0, unsaved_files, 0, 0);

    if (!tu)
    {
        std::cerr << "Could not create translation unit from " << argv[1] << std::endl;
        return -1;
    }

    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);

    WalkParams params = WalkParams::initial(CXCursor_VarDecl);
    clang_visitChildren(rootCursor, visitor, &params);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return 0;
}