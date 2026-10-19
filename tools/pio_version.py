# v1019z: real build version string in every env's bin.
# BOTCORPS_BUILD_ID = "v1019z <git short sha>[-dirty]", read at build time.
# A tree with no git metadata still builds: the id falls back to "v1019z UNKNOWN".
import subprocess
Import("env")  # noqa: F821 (PlatformIO SCons)
CUT = "v1019z"
FALLBACK = CUT + " UNKNOWN"

def _git(*a):
    try:
        r = subprocess.run(
            ["git"] + list(a),
            cwd=env["PROJECT_DIR"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            timeout=8,
            check=False,
        )
        if r.returncode != 0:
            return ""
        return r.stdout.decode(errors="replace").strip()
    except Exception:
        return ""

def _apply(bid):
    env.Append(CPPDEFINES=[("BOTCORPS_BUILD_ID", env.StringifyMacro(bid))])
    print("[botcorps] BOTCORPS_BUILD_ID = %s" % bid)

try:
    sha = _git("rev-parse", "--short=7", "HEAD") or "UNKNOWN"
    dirty = _git("status", "--porcelain", "--untracked-files=no")
    if sha != "UNKNOWN" and dirty:
        sha += "-dirty"
    _apply("%s %s" % (CUT, sha))
except Exception as exc:
    print("[botcorps] version fallback (%s)" % exc)
    _apply(FALLBACK)
