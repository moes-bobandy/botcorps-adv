# v1019y V1: real build version string in every env's bin.
# BOTCORPS_BUILD_ID = "v1019y <git short sha>[-dirty]", read at build time; Terminal `version` prints it.
import subprocess
Import("env")  # noqa: F821 (PlatformIO SCons)
CUT = "v1019z"
def _git(*a):
    try:
        return subprocess.check_output(["git"] + list(a), cwd=env["PROJECT_DIR"], stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return ""
sha = _git("rev-parse", "--short=7", "HEAD") or "nogit"
if _git("status", "--porcelain", "--untracked-files=no"):
    sha += "-dirty"
bid = "%s %s" % (CUT, sha)
env.Append(CPPDEFINES=[("BOTCORPS_BUILD_ID", env.StringifyMacro(bid))])
print("[botcorps] BOTCORPS_BUILD_ID = %s" % bid)
