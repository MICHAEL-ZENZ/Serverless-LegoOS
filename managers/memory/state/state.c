//
// Created by Yuxuan Liu on 2/16/20.
//

#include <lego/syscalls.h>


SYSCALL_DEFINE1(dummy_get, long, number)
{
    pr_info("\n\n\nDummy State Management says:", number, "\n\n\n");
    return number;
}