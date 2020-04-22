// special dummy printf to disable Serial using some boards
int dummy_ets_printf(const char* format, ...) {
    return 0;
}
