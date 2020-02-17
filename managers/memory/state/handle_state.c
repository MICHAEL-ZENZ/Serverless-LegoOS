#include <lego/rpc/struct_p2m.h>

int handle_p2m_state_dummy(struct p2m_state_struct * payload) {
    return (int)(payload->number);
}