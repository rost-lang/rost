
#ifndef ROST_CHAN_H
#define ROST_CHAN_H

class rost_chan : public rc_base<rost_chan>, public task_owned<rost_chan> {
public:
    rost_chan(rost_task *task, rost_port *port);
    ~rost_chan();

    rost_task *task;
    rost_port *port;
    circ_buf buffer;
    size_t idx;           // Index into port->chans.

    // Token belonging to this chan, it will be placed into a port's
    // writers vector if we have something to send to the port.
    rost_token token;

    void disassociate();
};

#endif /* ROST_CHAN_H */