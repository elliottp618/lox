#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "debug.h"
#include "scanner.h"

static char* readFile( const char* path ) {
    // open file
    FILE* file = fopen( path, "rb" );
    if( NULL == file ) {
        fprintf( stderr, "Could not open file \"%s\".\n", path );
        return NULL;
    }

    // determine file size TODO: handle failure cases
    fseek( file, 0L, SEEK_END );
    size_t fileSize = ftell( file );
    rewind( file );

    // allocate buffer to hold file
    char* buffer = (char*)malloc( fileSize + 1 );
    if( NULL == buffer ) {
        fclose( file );
        fprintf( stderr, "Not enough memory to read \"%s\".\n", path );
        return NULL;
    }

    // read entire file into buffer
    size_t bytesRead = fread( buffer, sizeof( char ), fileSize, file );
    if( bytesRead < fileSize ) {
        free( buffer );
        fclose( file );
        fprintf( stderr, "Could not read file \"%s\".\n", path );
        return NULL;
    }
    
    // mark buffer with null terminator
    buffer[bytesRead] = '\0';

    // close file
    fclose( file );
    return buffer;
}

static int runFile( const char* path ) {
    // read source file
    char* source = readFile( path );
    if( NULL == source ) return 74;

    // interpret source file
    InterpretResult result = interpret_source( source );

    // done
    free( source );
    return INTERPRET_COMPILE_ERROR == result ? 65 : INTERPRET_RUNTIME_ERROR == result ? 70 : 0;
}

int main( int argc, const char* argv[] ) {
    // dispatch on command's 1st character
    switch( argc > 1 ? argv[1][0] : '\0' ) {
        // run
        case 'r': {
            if( argc < 3 ) break;
            initVM();
            int result = runFile( argv[2] );
            freeVM();
            return result;
        }


        // shell
        case 's': {
            initVM();
            char line[1024];
            printf( "Welcome to Lox. Type 'exit' to quit.\n" );
            for(;;) {
                printf( "> " );
                if( !fgets( line, sizeof( line ), stdin ) ) { printf( "\n" ); break; }
                if( 0 == strcmp( "exit\n", line ) ) break;
                interpret_source( line );
            }
            freeVM();
            return 0;
        }

        // eval
        case 'e': {
            initVM();
            if( argc < 3 ) break;
            interpret_source( argv[2] );
            freeVM();
            return 0;
        }

        // test
        case 't': {
            // init virtual machine
            initVM();

            // init chunk
            Chunk chunk;
            initChunk( &chunk );
            
            // 1.2 + 3.4
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 1.2 ) ), 123 );
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 3.4 ) ), 123 );
            writeChunk( &chunk, OP_ADD, 123 );
            
            // divide by 5.6
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 5.6 ) ), 123 );
            writeChunk( &chunk, OP_DIVIDE, 123 );

            // negate & print
            writeChunk( &chunk, OP_NEGATE, 123 );
            writeChunk( &chunk, OP_PRINT, 123 );
            
            // write a constant and then pop it
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 6 ) ), 123 );
            writeChunk( &chunk, OP_POP, 123 );

            // write two strings
            // TODO: there's a bug here! we intern the strings OK, but they get added to the constant table in two different slots
            // (note that the same thing happens for numerical values, too)
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, OBJ_VAL( makeString( "hi", 2 ) ) ), 123 );
            writeChunk( &chunk, OP_CONSTANT, 123 );
            writeChunk( &chunk, (uint8_t)addConstant( &chunk, OBJ_VAL( makeString( "hi", 2 ) ) ), 123 );

            // concat them
            writeChunk( &chunk, OP_ADD, 123 );
            writeChunk( &chunk, OP_POP, 123 );
            writeChunk( &chunk, OP_RETURN, 123 );

            // disassemble
            disassembleChunk( &chunk, "disassemble chunk" );

            // interpret
            interpret_chunk( &chunk );
            freeChunk( &chunk );

            // now run something else
            // this should print 'true'
            InterpretResult result = interpret_source( "print !(5 - 4 > 3 * 2 == !nil);" );
            if( INTERPRET_OK != result ) fprintf( stderr, "test failed\n" );

            // test string interning (should have some addresses)
            ObjString* s1 = concatStrings( "hello", 5, " world", 6 );
            printString( s1 );
            printf( "\n" );

            ObjString* s2 = concatStrings( "hello", 5, " world", 6 );
            printString( s2 );
            printf( "\n" );

            ObjString* s3 = makeString( "hi", 2 );
            printString( s3 );
            printf( "\n" );

            // print table load (should be 2)
            printf( "vm.strings.load: %ld\n", vm.strings.load );

            // now delete 's3' and see what happens
            tableDelete( &vm.strings, s3 );

            // print table load (should be 2)
            printf( "after deleting 'hi' - vm.strings.load: %ld\n", vm.strings.load ); 

            // recreate string (new address because we removed it from the hash table)
            ObjString* s4 = makeString( "hi", 2 );
            printString( s4 );
            printf( "\n" );

            // load is still 2
            printf( "vm.strings.load: %ld\n", vm.strings.load );

            // test variable assignment precedence
            // this should evaluate to 10, because it's (a * (b = c)) + d according to precedence rules
            InterpretResult result2 = interpret_source( "var a = 2; var b = 2; var c = 3; var d = 4; print a * b = c + d;" );
            if( INTERPRET_OK != result2 ) fprintf( stderr, "test2 failed\n" );
           
            // done - release all the objects, which will include both versions of 'hi'
            freeVM();
            return 0;
        }
    }
    
    // unrecognized command
    fprintf( stderr, "Usage: clox [run {file}|shell|eval|test]\n" );
    return 64;
}
