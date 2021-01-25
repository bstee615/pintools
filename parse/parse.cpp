#include <clang-c/Index.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

// https://bastian.rieck.ru/blog/posts/2015/baby_steps_libclang_ast/
struct Location
{
    std::string filename;
    unsigned int lineno;

    Location(CXCursor cursor)
    {
        CXSourceLocation cxLocation = clang_getCursorLocation(cursor);
        CXFile file;
        unsigned int column, offset;
        clang_getExpansionLocation(cxLocation, &file, &lineno, &column, &offset);
        filename = std::string(clang_getCString(clang_getFileName(file)));
    }
};

/// The information we need at one level of the traversal.
class WalkParams
{
    unsigned int level;
    CXCursorKind seeking;
    CXCursor cursor;
    std::shared_ptr<std::vector<Location>> tags;

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
        if (cursorKind() == CXCursor_TranslationUnit)
        {
            return true;
        }
        CXSourceLocation location = clang_getCursorLocation(cursor);
        return (bool)clang_Location_isFromMainFile(location);
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

    void tag()
    {
        tags->push_back(Location(cursor));
    }

    void log()
    {
        std::cout << std::string(level, '-') << " ";
        std::cout << getCursorKindName() << " (" << getCursorSpelling() << ") ";
        std::cout << (cursorMatches() ? "* " : "");
        std::cout << "\n";
    }

    static CXChildVisitResult visitor(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
    {
        WalkParams currentParams(cursor, clientData);
        if (!currentParams.isValidLocation())
        {
            return CXChildVisit_Continue;
        }

        // If this is the level we want, tag it and log
        if (currentParams.cursorMatches())
        {
            currentParams.tag();
        }
        // currentParams.log();

        // Create next call's params and recurse
        WalkParams nextParams = WalkParams::next(currentParams);
        clang_visitChildren(cursor,
                            visitor,
                            &nextParams);
        return CXChildVisit_Continue;
    }

public:

    // Copy params from CXCursor and CXClientData during the traversal.
    WalkParams(CXCursor cursor, CXClientData data) : WalkParams(*reinterpret_cast<WalkParams *>(data))
    {
        this->cursor = cursor;
    }

    WalkParams(const WalkParams &currentParams) : level(currentParams.level),
                                                  seeking(currentParams.seeking),
                                                  cursor(currentParams.cursor),
                                                  tags(currentParams.tags) {}

    WalkParams(CXCursor rootCursor, CXCursorKind seeking) :cursor(rootCursor), level(0), seeking(seeking), tags(std::make_shared<std::vector<Location>>()) {}

    std::shared_ptr<std::vector<Location>> traverse() {
        visitor(cursor, CXCursor(), (CXClientData)this);
        return tags;
    }
};

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
    WalkParams params = WalkParams(rootCursor, CXCursor_VarDecl);
    auto tags = params.traverse();

    for (auto l : *tags)
    {
        std::cout << l.filename << " " << l.lineno << std::endl;
    }

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return 0;
}