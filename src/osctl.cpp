#include <calf/osctl.h>

const char *osctl::osc_type_name(osc_type type)
{
    switch(type)
    {
    case osc_i32: return "i32";
    case osc_i64: return "i64";
    case osc_f32: return "f32";
    case osc_ts: return "ts";
    case osc_string: return "str";
    case osc_string_alt: return "stralt";
    case osc_blob: return "blob";
    case osc_char: return "char";
    case osc_rgba: return "rgba";
    case osc_midi: return "midi";
    case osc_true: return "TRUE";
    case osc_false: return "FALSE";
    case osc_nil: return "NIL";
    case osc_inf: return "INF";
    case osc_start_array: return "[";
    case osc_end_array: return "]";
    default:
        return "unknown";
    }
}

