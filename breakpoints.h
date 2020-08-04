
struct breakpoint {
    const char *name;
    unsigned char *address;
    void (*handler)(_EXCEPTION_POINTERS *ExceptionInfo);
    unsigned char orig_byte;
};

void print_regs(_EXCEPTION_POINTERS *ExceptionInfo);
void set_bps(struct breakpoint *bps);
