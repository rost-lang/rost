/*
 * Logging infrastructure that aims to support multi-threading, indentation
 * and ansi colors.
 */

#include "rost_internal.h"

static uint32_t read_type_bit_mask() {
    uint32_t bits = rost_log::ULOG | rost_log::ERR;
    char *env_str = getenv("ROST_LOG");
    if (env_str) {
        bits = 0;
        bits |= strstr(env_str, "err") ? rost_log::ERR : 0;
        bits |= strstr(env_str, "mem") ? rost_log::MEM : 0;
        bits |= strstr(env_str, "comm") ? rost_log::COMM : 0;
        bits |= strstr(env_str, "task") ? rost_log::TASK : 0;
        bits |= strstr(env_str, "up") ? rost_log::UPCALL : 0;
        bits |= strstr(env_str, "dom") ? rost_log::DOM : 0;
        bits |= strstr(env_str, "ulog") ? rost_log::ULOG : 0;
        bits |= strstr(env_str, "trace") ? rost_log::TRACE : 0;
        bits |= strstr(env_str, "dwarf") ? rost_log::DWARF : 0;
        bits |= strstr(env_str, "cache") ? rost_log::CACHE : 0;
        bits |= strstr(env_str, "timer") ? rost_log::TIMER : 0;
        bits |= strstr(env_str, "all") ? rost_log::ALL : 0;
    }
    return bits;
}

rost_log::ansi_color rost_log::get_type_color(log_type type) {
    switch (type) {
    case ERR:
        return rost_log::RED;
    case UPCALL:
        return rost_log::GREEN;
    case COMM:
        return rost_log::MAGENTA;
    case DOM:
    case TASK:
        return rost_log::LIGHTTEAL;
    case MEM:
        return rost_log::YELLOW;
    default:
        return rost_log::WHITE;
    }
}

static const char * _foreground_colors[] = { "[30m", "[1;30m", "[37m",
                                             "[31m", "[1;31m", "[32m",
                                             "[1;32m", "[33m", "[33m",
                                             "[34m", "[1;34m", "[35m",
                                             "[1;35m", "[36m", "[1;36m" };
rost_log::rost_log(rost_srv *srv, rost_dom *dom) :
    _srv(srv), _dom(dom), _type_bit_mask(read_type_bit_mask()),
            _use_colors(getenv("ROST_COLOR_LOG")), _indent(0) {
}

rost_log::~rost_log() {

}

void rost_log::trace_ln(char *message) {
    char buffer[512];
    if (_use_colors) {
        snprintf(buffer, sizeof(buffer), "\x1b%s0x%08" PRIxPTR "\x1b[0m: ",
                 _foreground_colors[1 + ((uintptr_t) _dom % 2687 % (LIGHTTEAL
                         - 1))], (uintptr_t) _dom);
    } else {
        snprintf(buffer, sizeof(buffer), "0x%08" PRIxPTR ": ",
                 (uintptr_t) _dom);
    }

    for (uint32_t i = 0; i < _indent; i++) {
        strncat(buffer, "\t", sizeof(buffer) - strlen(buffer) - 1);
    }
    strncat(buffer, message, sizeof(buffer) - strlen(buffer) - 1);
    _srv->log(buffer);
}

/**
 * Traces a log message if the specified logging type is not filtered.
 */
void rost_log::trace_ln(uint32_t type_bits, char *message) {
    trace_ln(get_type_color((rost_log::log_type) type_bits), type_bits,
             message);
}

/**
 * Traces a log message using the specified ANSI color code.
 */
void rost_log::trace_ln(ansi_color color, uint32_t type_bits, char *message) {
    if (is_tracing(type_bits)) {
        if (_use_colors) {
            char buffer[512];
            snprintf(buffer, sizeof(buffer), "\x1b%s%s\x1b[0m",
                     _foreground_colors[color], message);
            trace_ln(buffer);
        } else {
            trace_ln(message);
        }
    }
}

bool rost_log::is_tracing(uint32_t type_bits) {
    return type_bits & _type_bit_mask;
}

void rost_log::indent() {
    _indent++;
}

void rost_log::outdent() {
    _indent--;
}

void rost_log::reset_indent(uint32_t indent) {
    _indent = indent;
}