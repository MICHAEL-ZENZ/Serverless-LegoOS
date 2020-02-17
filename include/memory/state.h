//
// Created by Yuxuan Liu on 2/16/20.
//

#ifndef _LEGO_MEMORY_STATE_H
#define _LEGO_MEMORY_STATE_H

#include <lego/slab.h> // kmalloc
#include <lego/fit_ibapi.h>
#include <lego/syscalls.h>
#include <lego/rpc/struct_p2m.h>

static ssize_t p2m_state(long number)
{
    ssize_t retval, retlen;
    ssize_t *retval_ptr;
    u32 len_retbuf, len_msg;
    void *retbuf, *msg;
    struct common_header *hdr;
    struct p2m_state_struct *payload;
    int mem_node;

    len_retbuf = sizeof(ssize_t);
    retbuf = kmalloc(len_retbuf, GFP_KERNEL);
    if (!retbuf)
        return -ENOMEM;

    len_msg = sizeof(*hdr) + sizeof(*payload);
    msg = kmalloc(len_msg, GFP_KERNEL);
    if (!msg) {
        kfree(retbuf);
        return -ENOMEM;
    }

    /* Construct payload */
    hdr = msg;
    hdr->opcode = P2M_STATE_DUMMY;
    hdr->src_nid = LEGO_LOCAL_NID;

    payload = msg + sizeof(*hdr);
    payload->number = number;

    mem_node = current_memory_home_node();
    retlen = ibapi_send_reply_imm(mem_node, msg, len_msg,
                                  retbuf, len_retbuf, false);

    if (retlen != len_retbuf) {
        WARN_ON_ONCE(1);
        retval = -EIO;
        goto out;
    }

    retval_ptr = retbuf;
    retval = *retval_ptr;

    return retval;

}


#endif //_LEGO_MEMORY_STATE_H