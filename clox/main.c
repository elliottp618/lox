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
    Value value = interpret( source );

    // done
    free( source );
    return IS_ERROR( value ) ? 65 : 0; // return INTERPRET_COMPILE_ERROR == result ? 65 : INTERPRET_RUNTIME_ERROR == result ? 70 : 0;
}

int main( int argc, const char* argv[] ) {
    // dispatch on command's 1st character
    switch( argc > 1 ? argv[1][0] : '\0' ) {
        // run file
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
            printf( "Welcome to Lox. Type 'q' to quit.\n" );
            for(;;) {
                printf( "> " );
                if( !fgets( line, sizeof( line ), stdin ) ) { printf( "\n" ); break; }
                if( 0 == strcmp( "q\n", line ) ) break;
                Value value = interpret( line );
                printf( "=> result: " );
                printValue( value );
                printf( "\n" );
            }
            freeVM();
            return 0;
        }

        // eval
        case 'e': {
            initVM();
            if( argc < 3 ) break;
            interpret( argv[2] );
            freeVM();
            return 0;
        }

        // test
        case 't': {
            // TEST
            {
                printf( "\n=> TEST -((1.2 + 3.4) / 2)\n" );

                // init VM
                initVM();
                Chunk chunk;
                initChunk( &chunk );

                // write chunk
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 1.2 ) ), 123 );
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 3.4 ) ), 123 );
                writeChunk( &chunk, OP_ADD, 123 );
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, NUMBER_VAL( 2 ) ), 123 );
                writeChunk( &chunk, OP_DIVIDE, 123 );
                writeChunk( &chunk, OP_NEGATE, 123 );
                writeChunk( &chunk, OP_RETURN, 123 );

                // interpret
                printf( "=> bytecode\n" );
                disassembleChunk( &chunk );
                Value value = interpret_chunk( &chunk );

                // validate
                if( IS_NUMBER( value ) && valuesEqual( value, NUMBER_VAL( -2.3 ) ) ) {
                    printf( "SUCCESS\n" );
                } else {
                    printf( "ERROR: Expected -2.3, but got: " );
                    printValue( value );
                    printf( "\n" );
                    freeVM();
                    return 1;
                }

                // free VM
                freeChunk( &chunk );
                freeVM();
            }

            // TEST
            {
                printf( "\n=> TEST intern & concat 2 identical strings\n" );

                // init VM
                initVM();
                Chunk chunk;
                initChunk( &chunk );

                // write chuink
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, OBJ_VAL( makeString( "hi", 2 ) ) ), 123 );
                writeChunk( &chunk, OP_CONSTANT, 123 );
                writeChunk( &chunk, (uint8_t)addConstant( &chunk, OBJ_VAL( makeString( "hi", 2 ) ) ), 123 );
                writeChunk( &chunk, OP_ADD, 123 );
                writeChunk( &chunk, OP_RETURN, 123 );
                printf( "=> bytecode\n" );
                disassembleChunk( &chunk );
                Value value = interpret_chunk( &chunk );

                // validate
                if( IS_STRING( value ) && valuesEqual( value, OBJ_VAL( makeString( "hihi", 4 ) ) ) ) {
                    printf( "SUCCESS (note: string interned OK, but constant is still duped!)\n" );
                } else {
                    printf( "ERROR: Expected 'hihi', but got: " );
                    printValue( value );
                    printf( "\n" );
                    freeVM();
                    return 1;
                }

                // free VM
                freeChunk( &chunk );
                freeVM();
            }

            // TEST
            {
                printf( "\n=> TEST !(5 - 4 > 3 * 2 == !nil)\n" );
                initVM();
                Value value = interpret( "return !(5 - 4 > 3 * 2 == !nil);" );
                if( IS_BOOL( value ) && valuesEqual( value, BOOL_VAL( true ) ) ) {
                    printf( "SUCCESS\n" );
                } else {
                    printf( "ERROR: Expected 'true', but got: " );
                    printValue( value );
                    printf( "\n" );
                    freeVM();
                    return 1;
                }
                freeVM();
            }

            // TEST
            {
                printf( "\n=> TEST STRING INTERNING\n" );
                initVM();

                // create string objects "hello world" and "hi"
                size_t init_load = vm.strings.load;
                concatStrings( "hello", 5, " world", 6 );
                concatStrings( "hello", 5, " world", 6 );
                makeString( "hi", 2 );

                // test # of strings we actually created in the VM - it should just be two
                if( 2 == vm.strings.load - init_load ) {
                    printf( "SUCCESS\n" );
                } else {
                    printf( "ERROR: Expected 2 strings, but got: %zu strings\n", vm.strings.load );
                    freeVM();
                    return 1;
                }
                freeVM();
            }

            // test variable assignment precedence
            {
                initVM();
                printf( "\n=> TEST ASSIGNMENT PRECEDENCE: var x = 1; return x = 3 + 4;\n" );
                Value value = interpret( "var x = 1; return x = 3 + 4;" ); // should be fine since 'return' is PRECEDENCE_NONE which is above assignment
                if( IS_NUMBER( value ) && valuesEqual( value, NUMBER_VAL( 7 ) ) ) {
                    printf( "SUCCESS\n" );
                } else {
                    printf( "ERROR: expected 7, but got: " );
                    printValue( value );
                    printf( "\n" );
                    freeVM();
                    return 1;
                }
                freeVM();
            }

            // test incorrect variable assignment precedence
            {
                initVM();
                printf( "\n=> TEST INCORRECT ASSIGNMENT PRECEDENCE: var x = 1; return 2 * x = 3 + 4;\n" );
                Value value = interpret( "var x = 1; return 2 * x = 3 + 4;" ); // should be fine since 'return' is PRECEDENCE_NONE which is above assignment
                if( IS_ERROR( value ) && valuesEqual( value, ERROR_VAL( COMPILE_ERROR ) ) ) {
                    printf( "SUCCESS\n" );
                } else {
                    printf( "ERROR: expected COMPILE_ERROR, but got: " );
                    printValue( value );
                    printf( "\n" );
                    freeVM();
                    return 1;
                }
                freeVM();
            }

            // test creation & access of local varibles
            // TODO

            /*
            // test creation of local variables
            result = interpret( "{ var x = 5; var y = 6; }" );
            if( INTERPRET_OK != result ) fprintf( stderr, "local varible test failed\n" );

            // test accessing local variable
            result = interpret( "{ var x = 8; print x; }" );
            if( INTERPRET_OK != result ) fprintf( stderr, "accessing local varible test failed\n" );

            // test re-definition of local variable
            result = interpret( "{ var x = 5; var x = 6; }" );
            if( INTERPRET_COMPILE_ERROR != result ) fprintf( stderr, "variable redefinition test failed\n" );

            // test accessing variable within its own initializer
            result = interpret( "{ var x = x; }" );
            if( INTERPRET_COMPILE_ERROR != result ) fprintf( stderr, "error - variable allowed to access itself within its initializer! (reads stale value from stack, which is leftover from VM's last execution)\n" );

            // test initializing variable with a variable of same name but higher scope
            // note that LOX does not support this (at the moment), but it probably should eventually
            result = interpret( "var x = 1; { var x = x; print x; }" );
            if( INTERPRET_COMPILE_ERROR != result ) fprintf( stderr, "error - was able to initialize varible with a same-named higher-scoped variable\n" );

            // test a simple if statement
            result = interpret( "if( true ) print \"CORRECT!\"; if( false ) print \"ERROR!\";" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile a set of if statements\n" );

            // test if-else statements
            result = interpret( "if( true ) print \"CORRECT!\"; else print \"ERROR!\"; if( false ) print \"ERROR!\"; else print \"CORRECT!\";" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile a set of if-else statements\n" );

            // logical and
            result = interpret( "if( true and false ) print \"ERROR!\"; else print \"CORRECT\";" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile logical and\n" );

            // logical or
            result = interpret( "if( true or false ) print \"CORRECT!\"; else print \"ERROR\";" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile logical or\n" );

            // while
            result = interpret( "{ var i = 0; while( i < 3 ) { print i; i = i + 1; } }" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile while statement\n" );

            // for loop (no content)
            result = interpret( "for( var i = 0; i < 3; i = i + 1 ) print i;" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile for statement\n" );

            // function declaration
            result = interpret( "fun say_hi() { print \"hi\"; } print say_hi;" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile function declaration\n" );

            // function declaration w/ parameters
            result = interpret( "fun say_hi( text1, text2 ) { print text1; print text2; } print say_hi;" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - could not compile function declaration w/ parameters\n" );

            // function call w/ 1 argument, but CANNOT return yet, so it'll never print the my_func( 7 )!
            result = interpret( "fun my_func( x ) { print x; } my_func( 6 ); my_func( 7 );" );
            if( INTERPRET_RUNTIME_ERROR == result ) fprintf( stderr, "error - could not execute function call w/ parameter\n" );

            // function call w/ 1 argument, but CANNOT return yet, so it'll never print the my_func( 7 )!
            result = interpret( "fun my_func( x ) { print x; } my_func( 6, 7 );" );
            if( INTERPRET_RUNTIME_ERROR != result ) fprintf( stderr, "error - wrong # of function params should throw an error, but did not\n" );

            // broken program for testing stack-trace printing
            result = interpret(
                "fun a() { b(); }\n"
                "fun b() { c(); }\n"
                "fun c() {\n"
                "   c(\"too\", \"many\");\n"
                "}\n"
                "\n"
                "a();\n" );
            if( INTERPRET_RUNTIME_ERROR != result ) fprintf( stderr, "error - should have crashed due too a nested call w/ too many args" );

            // function which actually returns something
            result = interpret( "fun add1( x ) { return x + 1; } print add1( 2 ); " );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - should have been able to interpret simple function w/ return value\n" );

            // test a native 'clock' function
            result = interpret( "print clock();" );
            if( INTERPRET_OK != result ) fprintf( stderr, "error - should have been able to run clock program\n" );
            */

            // done
            return 0;
        }
    }
    
    // unrecognized command
    fprintf( stderr, "Usage: clox [run {file}|shell|eval|test]\n" );
    return 64;
}
