# The Core links printf / scanf with floating point support (-u _printf_float -u _scanf_float, ~12 KB):
# nothing here prints or parses a float
Import("env")

flags = env["LINKFLAGS"]
for symbol in ("_printf_float", "_scanf_float"):
    for index in range(len(flags) - 1):
        if (flags[index] == "-u") and (flags[index + 1] == symbol):
            del flags[index:index + 2]
            break

