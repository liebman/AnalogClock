Import('env')

# the uplaod flags for attiny85 adds -e that erases the flash and when we don't
# want that to happen when we are setting the fuses, espically when we change
# the reset pin to an IO pin.  The '-e' is actually added twice, once via extra
# upload flags in the board definition and once in the setup for the fuses target
def fuses_command(source, target, env):
    env['UPLOADERFLAGS'].remove('-e')
    cmd = " ".join(
        ["avrdude", "$UPLOADERFLAGS"] +
        ["-U%s:w:%s:m" % (k, v)
        for k, v in env.BoardConfig().get("fuses", {}).items()]
    )
    return env.Execute(cmd)

env.Replace(FUSESCMD=fuses_command)
