
#include <windows.h>
#include <stdio.h>

#include "breakpoints.h"

LPTOP_LEVEL_EXCEPTION_FILTER old_filter;

#define BP_COUNT (sizeof(breakpoints)/sizeof(*breakpoints))

static struct breakpoint *breakpoints;

void
print_regs(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    ULONG64 *rsp = (ULONG64 *)ctx->Rsp;
    size_t i;

    // ExceptionInfo->ExceptionRecord->ExceptionAddress
    printf("============================================ Breakpoint at %p ===================================\n", ctx->Rip);
    printf("RSP = %016llx\n", ctx->Rsp);
    printf("RAX = %016llx\n", ctx->Rax);
    printf("RCX = %016llx\n", ctx->Rcx);
    printf("RDX = %016llx\n", ctx->Rdx);
    printf("R8  = %016llx\n", ctx->R8);
    printf("R9  = %016llx\n", ctx->R9);

    printf("Stack:\n");
    for(i=0;i<40;i++) {
        printf("    %016llx\n", rsp[i]);
    }
}

LONG 
BreakpointExceptionFilter(_EXCEPTION_POINTERS *ExceptionInfo)
{
    static struct breakpoint *last_hit;
    DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    unsigned char *addr = (unsigned char *)ExceptionInfo->ExceptionRecord->ExceptionAddress;

    puts("trap");

    if(code == EXCEPTION_BREAKPOINT) {
        for(struct breakpoint *b = breakpoints;b->address;b++)
            if(b->address == addr) {
                last_hit = b;
                *addr = b->orig_byte;
                if(b->name)
                    printf("=========================== %s =======================\n", b->name);
                b->handler(ExceptionInfo);
                ExceptionInfo->ContextRecord->EFlags |= 0x100; // enable single step execution
                SetThreadContext(GetCurrentThread(), ExceptionInfo->ContextRecord); // non-return
            }

    }
    else if(code == EXCEPTION_SINGLE_STEP) {
        if(last_hit && addr-last_hit->address < 100) {
            // restore last breakpoint
            *last_hit->address = 0xcc;
            last_hit = NULL;
        }
        else {
            printf("EXCEPTION_SINGLE_STEP: %p\n", addr);
            print_regs(ExceptionInfo);
        }
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
        printf("Setting bp at %p, old byte=%02x (%s)\n", b->address, b->orig_byte, b->name);
    }
}

