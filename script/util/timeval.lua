local ffi = require "ffi"

ffi.cdef [[

struct timeval {
    long tv_sec;
    long tv_usec;
};

]]

