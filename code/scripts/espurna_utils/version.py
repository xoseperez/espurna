import os
import functools
import subprocess

from .display import print_warning


try:
    cached = functools.cache
except AttributeError:
    cached = functools.lru_cache(None)


@cached
def app_revision():
    def git(*args):
        cmd = ["git"]
        cmd.extend(args)

        result = subprocess.run(cmd, capture_output=True, universal_newlines=True)
        result.check_returncode()

        return result.stdout.strip()

    revision = None
    try:
        revision = git("rev-parse", "--short=8", "HEAD")
    except subprocess.CalledProcessError:
        pass
    except FileNotFoundError:
        pass

    return revision


@cached
def app_version(version_h):
    version = None
    with open(version_h, "r") as f:
        for line in f:
            if "define" in line and "APP_VERSION" in line:
                version = line.split(" ")[-1]
                version = version.strip().replace('"', "")
                break

    return version


def app_version_for_env(env):
    return env.get("ESPURNA_BUILD_VERSION") or app_version(
        os.path.join(env.get("PROJECT_DIR"), "espurna/config/version.h")
    )


def app_revision_for_env(env):
    return env.get("ESPURNA_BUILD_REVISION") or app_revision()


def app_suffix_for_env(env):
    return env.get("ESPURNA_BUILD_VERSION_SUFFIX", "")


def app_combined_version(env):
    version = app_version_for_env(env)
    if not version:
        raise ValueError("Version string cannot be empty")

    revision = app_revision_for_env(env)
    if revision:
        # handle both 1.2.3-dev.git... and 1.2.3-git...
        # and avoid 1.2.3.git... that cannot be parsed by the semantic_version module
        middle = ".git" if "-" in version else "-git"
        version = middle.join([version, revision])

    suffix = app_suffix_for_env(env)
    if suffix:
        version = "+".join([version, suffix])

    return version


def app_full_version_for_env(env):
    return env.get("ESPURNA_BUILD_FULL_VERSION") or app_combined_version(env)


def app_inject_version(env):
    def inject_string(env, flag, value):
        env.Append(CPPDEFINES=[(flag, '\\"{}\\"'.format(value))])

    inject_string(env, "APP_VERSION", app_full_version_for_env(env))
