# Beken: the prebuilt Matter library of github.com/igorufox/matter-bk7231n
#
# connectedhomeip built for the BK7231N SDK with its own GN / ninja setup, not something PlatformIO can build. It comes
# from a release asset of matter-bk7231n, a zip of
#   lib/libMatterApp.a
#   include/lwipopts.h   (lwIP options the library was built with: the SDK's lwIP has to be built with the same)
# checked against its SHA-256 and unpacked into .pio/matter-lib/<hash>:
#
#   extra_scripts = pre:scripts/beken_matter_lib.py
#   custom_matter_lib = latest
#
# `latest` (or `github:<owner>/<repo>@latest`) is the newest libmatter-* release of matter-bk7231n (its GitHub
# Actions build one after every change), found through the GitHub API (GITHUB_TOKEN is used when set: 60 requests an
# hour without it) and checked against the matter-bk7231n-lib.zip.sha256 asset of that release. Without network the
# last one resolved is used again. A fixed library instead:
#
#   custom_matter_lib = <URL of the zip>
#   custom_matter_lib_sha256 = <SHA-256 of the zip>
#
# ESPURNA_MATTER_LIB_DIR=<folder with lib/ and include/> uses a local build instead (development of the library).
# A pre: script, so that the include folder reaches the SDK libraries built by the platform too.

import hashlib
import io
import json
import os
import re
import urllib.request
import zipfile

Import("env")  # noqa: F821

FILES = ("lib/libMatterApp.a", "include/lwipopts.h")

LATEST_REPO = "igorufox/matter-bk7231n"
LATEST_TAG = "libmatter-"
ASSET = "matter-bk7231n-lib.zip"


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def unpacked(folder):
    return all(os.path.isfile(os.path.join(folder, name)) for name in FILES)


def download(url, headers=None, timeout=120):
    request = urllib.request.Request(url, headers={"User-Agent": "espurna-build", **(headers or {})})
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return response.read()


def fetch(url, expected, folder):
    if unpacked(folder):
        return

    print(f"Matter library: {url}")
    data = download(url)

    actual = sha256(data)
    if actual != expected:
        raise ValueError(f"Matter library SHA-256 mismatch: {actual}, expected {expected}")

    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        for name in FILES:
            target = os.path.join(folder, name)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            with open(target + ".part", "wb") as out:
                out.write(archive.read(name))
            os.replace(target + ".part", target)


def latest_repo(spec):
    # `latest` or `github:<owner>/<repo>@latest`, None for an URL
    if spec == "latest":
        return LATEST_REPO
    match = re.fullmatch(r"github:([\w.-]+/[\w.-]+)@latest", spec)
    return match.group(1) if match else None


def resolve_latest(repo):
    # -> (tag, URL of the zip, SHA-256 of the zip) of the newest libmatter-* release with both assets
    headers = {"Accept": "application/vnd.github+json", "X-GitHub-Api-Version": "2022-11-28"}
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"

    releases = json.loads(download(f"https://api.github.com/repos/{repo}/releases?per_page=50", headers, 30))
    releases = [
        release
        for release in releases
        if release["tag_name"].startswith(LATEST_TAG) and not release["draft"] and not release["prerelease"]
    ]
    releases.sort(key=lambda release: release.get("published_at") or "", reverse=True)

    for release in releases:
        assets = {asset["name"]: asset["browser_download_url"] for asset in release["assets"]}
        if ASSET in assets and f"{ASSET}.sha256" in assets:
            line = download(assets[f"{ASSET}.sha256"], timeout=30).decode("ascii", "replace")
            match = re.match(r"\s*([0-9a-fA-F]{64})\b", line)
            if not match:
                raise ValueError(f"Matter library: no SHA-256 in {ASSET}.sha256 of {release['tag_name']}")
            return release["tag_name"], assets[ASSET], match.group(1).lower()

    raise ValueError(f"Matter library: no {LATEST_TAG}* release with {ASSET} in github.com/{repo}")


def library_dir(env):
    local = os.environ.get("ESPURNA_MATTER_LIB_DIR")
    if local:
        if not unpacked(local):
            raise ValueError(f"ESPURNA_MATTER_LIB_DIR={local} needs {' and '.join(FILES)}")
        return local

    spec = env.GetProjectOption("custom_matter_lib", "").strip()
    cache = os.path.join(env.subst("$PROJECT_DIR"), ".pio", "matter-lib")

    repo = latest_repo(spec)
    if repo:
        state = os.path.join(cache, "latest.json")
        try:
            tag, url, expected = resolve_latest(repo)
        except Exception as error:
            # offline / API rate limit: the last release resolved, if it is still unpacked
            try:
                with open(state) as f:
                    last = json.load(f)
                folder = os.path.join(cache, last["sha256"][:16])
                if last.get("repo") == repo and unpacked(folder):
                    print(f"Matter library: {error}; using {last['tag']} resolved before")
                    return folder
            except (OSError, ValueError, KeyError):
                pass
            raise

        print(f"Matter library: {tag} of github.com/{repo}")
        folder = os.path.join(cache, expected[:16])
        fetch(url, expected, folder)
        with open(state + ".part", "w") as f:
            json.dump({"repo": repo, "tag": tag, "url": url, "sha256": expected}, f)
        os.replace(state + ".part", state)
        return folder

    expected = env.GetProjectOption("custom_matter_lib_sha256", "").lower()
    if not spec or not expected:
        raise ValueError("custom_matter_lib = latest, or custom_matter_lib (URL) and custom_matter_lib_sha256, are required for a Matter build")

    folder = os.path.join(cache, expected[:16])
    fetch(spec, expected, folder)
    return folder


if env.GetProjectOption("custom_matter_lib", "") or os.environ.get("ESPURNA_MATTER_LIB_DIR"):
    folder = library_dir(env)
    # first in the include path: our lwipopts.h instead of the SDK one, for every library built after this
    env.Prepend(CPPPATH=[os.path.join(folder, "include")])
    # same as `-L<folder> -lMatterApp` in build_flags: LIBS come after the objects of the link
    env.Append(LIBPATH=[os.path.join(folder, "lib")], LIBS=["MatterApp"])
