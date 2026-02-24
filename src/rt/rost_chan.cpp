
#include "rost_internal.h"
#include "rost_chan.h"

rost_chan::rost_chan(rost_task *task, rost_port *port) :
    task(task),
    port(port),
    buffer(task->dom, port->unit_sz),
    token(this)
{
    if (port)
        port->chans.push(this);
}

rost_chan::~rost_chan()
{
    if (port) {
        if (token.pending())
            token.withdraw();
        port->chans.swapdel(this);
    }
}

void
rost_chan::disassociate()
{
    I(task->dom, port);

    if (token.pending())
        token.withdraw();

    // Delete reference to the port/
    port = NULL;
}