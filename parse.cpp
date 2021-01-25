#include <clang-c/Index.h>

// https://bastian.rieck.ru/blog/posts/2015/baby_steps_libclang_ast/
int main( int argc, char** argv )
{
    if( argc < 2 )
    return -1;

    CXIndex index        = clang_createIndex( 0, 1 );
    CXTranslationUnit tu = clang_createTranslationUnit( index, argv[1] );

    if( !tu )
    return -1;

    CXCursor rootCursor  = clang_getTranslationUnitCursor( tu );

    clang_disposeTranslationUnit( tu );
    clang_disposeIndex( index );
    return 0;
}