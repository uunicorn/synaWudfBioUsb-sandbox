
struct breakpoint {
    unsigned char *address;
    void (*handler)(_EXCEPTION_POINTERS *ExceptionInfo);
    unsigned char orig_byte;
};

void set_bps(struct breakpoint *bps);
