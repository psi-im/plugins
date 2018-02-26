#include "signal_protocol.h"

int my_load_session_func(signal_buffer **record, const signal_protocol_address *address, void *user_data)
{
}

int main()
{
    signal_protocol_session_store store;
    store.load_session_func = my_load_session_func;
}
