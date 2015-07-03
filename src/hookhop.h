#ifndef __HOOK_HOP_H
#define __HOOK_HOP_H

// a class that replaces the first 5 bytes of a function with
// mov edi,edi
// push ebp
// mov ebp,esp
#pragma warning( push )
#pragma warning( disable : 4701 4731 4740 )
class HookHop
{
public:
    HookHop()
    {
    }

    ~HookHop()
    {
    }

    static void * FunctionHop( void *function )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }

    template<typename T>
    static void * FunctionHop( void *function, T a )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters
            push a

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }

    template<typename T, typename U>
    static void * FunctionHop( void *function, T a, U b )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters
            push b
            push a

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }

    template<typename T, typename U, typename V>
    static void * FunctionHop( void *function, T a, U b, V c )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters
            push c
            push b
            push a

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }

    template<typename T, typename U, typename V, typename W>
    static void * FunctionHop( void *function, T a, U b, V c, W d )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters
            push d
            push c
            push b
            push a

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }

    template<typename T, typename U, typename V, typename W, typename X>
    static void * FunctionHop( void *function, T a, U b, V c, W d, X e )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters
            push e
            push d
            push c
            push b
            push a

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }

    template<typename T, typename U, typename V, typename W, typename X, typename Y>
    static void * FunctionHop( void *function, T a, U b, V c, W d, X e, Y f )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters
            push f
            push e
            push d
            push c
            push b
            push a

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }

    template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z>
    static void * FunctionHop( void *function, T a, U b, V c, W d, X e, Y f, Z g )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters
            push g
            push f
            push e
            push d
            push c
            push b
            push a

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }

    template<typename S, typename T, typename U, typename V, typename W, typename X, typename Y, typename Z>
    static void * FunctionHop( void *function, S a, T b, U c, V d, W e, X f, Y g, Z h )
    {
        __asm
        {
            mov  eax,function // temporary holder for function
            add  eax,5   // function offset

            //parameters
            push h
            push g
            push f
            push e
            push d
            push c
            push b
            push a

            push ReturnLocation
            mov  edi,edi // first 5 bytes of the function
            push ebp
            mov  ebp,esp
            jmp  eax
    ReturnLocation:
        }
    }
};
#pragma warning( pop )

#endif //__HOOK_HOP_H