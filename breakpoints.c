
#include <windows.h>
#include <stdio.h>

#include "breakpoints.h"

LPTOP_LEVEL_EXCEPTION_FILTER old_filter;

#define BP_COUNT (sizeof(breakpoints)/sizeof(*breakpoints))

static struct breakpoint *breakpoints;

LONG 
BreakpointExceptionFilter(_EXCEPTION_POINTERS *ExceptionInfo)
{
    static struct breakpoint *last_hit;
    DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    unsigned char *addr = (unsigned char *)ExceptionInfo->ExceptionRecord->ExceptionAddress;

    if(code == EXCEPTION_BREAKPOINT) {
        for(struct breakpoint *b = breakpoints;b->address;b++)
            if(b->address == addr) {
                last_hit = b;
                *addr = b->orig_byte;
                b->handler(ExceptionInfo);
                ExceptionInfo->ContextRecord->EFlags |= 0x100; // enable single step execution
                SetThreadContext(GetCurrentThread(), ExceptionInfo->ContextRecord); // non-return
            }

    }
    else if(code == EXCEPTION_SINGLE_STEP) {
        // restore last breakpoint
        *last_hit->address = 0xcc;
        // single step execution is disabled automatically
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if(old_filter)
        return old_filter(ExceptionInfo);

    return EXCEPTION_CONTINUE_SEARCH;
}

void
set_bps(struct breakpoint *bps)
{
    if(breakpoints) {
        puts("Oops, set_bps was never meant to be called twice");
        return;
    }

    old_filter = SetUnhandledExceptionFilter(BreakpointExceptionFilter);
    breakpoints = bps;

    for(struct breakpoint *b = breakpoints;b->address;b++) {
        DWORD oldProtection;

        // enable writing to the text segment
        VirtualProtect(b->address, 1, PAGE_EXECUTE_READWRITE, &oldProtection);
        // backup the first byte of original instruction
        b->orig_byte = *b->address;
        // int3
        *b->address = 0xcc;
        printf("Setting bp at %p, old byte=%02x\n", b->address, b->orig_byte);
    }
}

