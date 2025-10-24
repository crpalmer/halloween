#include "pi.h"
#include "gp-input.h"
#include "gp-output.h"
#include "animation-setup.h"

#if WEEN_BOARD

#include "wb.h"

static WeenBoard *wb;

static void ensure_wb() {
    if (! wb) wb = new WeenBoard();
}

Input *prop_get_input(int prop_num) {
    ensure_wb();
    Input *input = wb->get_input(prop_num);
    input->set_pullup_down();
    return input;
}

Output *prop_get_light(int prop_num) {
    ensure_wb();
    return wb->get_output(1, prop_num);
}

Output *prop_get_output(int prop_num) {
    ensure_wb();
    return wb->get_output(2, prop_num);
}

#else

#include "mcp23017.h"

static MCP23017 *mcp;

static void ensure_mcp() {
    if (! mcp) mcp = new MCP23017();
}

Input *prop_get_input(int prop_num) {
    return new GPInput(4 + prop_num - 1);
}

Output *prop_get_light(int prop_num) {
    ensure_mcp();
    return mcp->get_output(1, prop_num);
}

Output *prop_get_output(int prop_num) {
    ensure_mcp();
    return mcp->get_output(2, prop_num);
}

Input *prop_get_extra_input(int prop_num) {
    ensure_mcp();
    return NULL;
}

#endif
