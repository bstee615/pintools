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
class ASTVisitor
{
    unsigned int level;
    CXCursorKind seeking;
    CXCursor cursor;
    std::vector<Location> *tags;

    // Getter for the current CXCursorKind.
    CXCursorKind cursorKind()
    {
        return clang_getCursorKind(cursor);
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

    // Copy params from CXCursor and CXClientData during the traversal.
    ASTVisitor(CXCursor cursor, CXClientData data) : ASTVisitor(*reinterpret_cast<ASTVisitor *>(data))
    {
        this->cursor = cursor;
    }

    ASTVisitor(const ASTVisitor &currentParams) : level(currentParams.level),
                                                  seeking(currentParams.seeking),
                                                  cursor(currentParams.cursor),
                                                  tags(std::move(currentParams.tags)) {}

    ASTVisitor(CXCursorKind seeking, std::vector<Location> *tags) : level(0), seeking(seeking), tags(tags) {}

    static ASTVisitor next(const ASTVisitor &currentParams)
    {
        ASTVisitor next(currentParams);
        next.level++;
        return next;
    }

    static CXChildVisitResult visit(CXCursor cursor, CXCursor /* parent */, CXClientData clientData)
    {
        ASTVisitor currentParams(cursor, clientData);
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
        ASTVisitor nextParams = ASTVisitor::next(currentParams);
        clang_visitChildren(cursor,
                            visit,
                            &nextParams);
        return CXChildVisit_Continue;
    }
    
    void tag()
    {
        tags->push_back(Location(cursor));
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

    // Return whether the cursor matches the kind we are seeking.
    bool cursorMatches()
    {
        return cursorKind() == seeking;
    }

    void log()
    {
        std::cout << std::string(level, '-') << " ";
        std::cout << getCursorKindName() << " (" << getCursorSpelling() << ") ";
        std::cout << (cursorMatches() ? "* " : "");
        std::cout << "\n";
    }

public:

    static std::vector<Location> Traverse(CXCursor rootCursor, CXCursorKind seeking) {
        std::vector<Location> tags;
        ASTVisitor visitor(seeking, &tags);
        visit(rootCursor, CXCursor(), (CXClientData)&visitor);
        return tags;
    }

    static std::vector<Location> Traverse(CXCursor rootCursor, std::vector<CXCursorKind> seeking) {
        std::vector<Location> allTags;
        for (auto s : seeking) {
            auto tags = Traverse(rootCursor, s);
            allTags.insert(allTags.end(), tags.begin(), tags.end());
        }
        return allTags;
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
    auto varDecls = ASTVisitor::Traverse(rootCursor, CXCursor_VarDecl);
    std::cout << "Variable declaration:" << std::endl;
    for (auto l : varDecls)
    {
        std::cout << l.filename << ":" << l.lineno << std::endl;
    }

    auto switchDecls = ASTVisitor::Traverse(rootCursor, {CXCursor_CaseStmt, CXCursor_DefaultStmt});
    std::cout << "Switch cases:" << std::endl;
    for (auto l : switchDecls)
    {
        std::cout << l.filename << ":" << l.lineno << std::endl;
    }

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return 0;
}