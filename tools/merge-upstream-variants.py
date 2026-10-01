#!/usr/bin/env python3
"""Merge upstream Gradle Module Metadata variants into this Maven branch.

This branch publishes the EGL-patched skiko under the upstream coordinates, which
means Gradle picks it up for every consumer of org.jetbrains.skiko:skiko.  A root
module whose metadata only lists the variants hosted here shadows the complete
upstream metadata and breaks every other target (ios, js, wasm, awt, ...) with
"no matching variant", because module metadata selection never falls back to the
next repository.

This tool keeps the variants hosted in this repository (metadata + linuxX64) and
re-emits all other upstream variants with an ABSOLUTE `available-at` URL pointing
at Maven Central (relative URLs would resolve back into this repository, where the
files do not exist).  Verified: Gradle 9 follows absolute available-at URLs.

Usage:
    tools/merge-upstream-variants.py [root] [upstream-metadata-url-or-path]

`root` defaults to the repository root (the directory containing org/).  Run it
after refreshing artifacts, before committing.
"""

from __future__ import annotations

import json
import pathlib
import sys
import urllib.parse
import urllib.request

UPSTREAM_DEFAULT = (
    "https://repo.maven.apache.org/maven2/org/jetbrains/skiko/skiko/0.150.1/skiko-0.150.1.module"
)
METADATA_REL = "org/jetbrains/skiko/skiko/0.150.1/skiko-0.150.1.module"

# Variants whose files are actually hosted in this repository.
HOSTED_PREFIXES = ("metadata", "linuxX64")


def load(source: str) -> dict:
    if source.startswith(("http://", "https://")):
        with urllib.request.urlopen(source) as response:
            return json.load(response)
    return json.loads(pathlib.Path(source).read_text())


def absolutize(variant: dict, base: str) -> dict:
    available_at = variant.get("available-at")
    if available_at is not None:
        available_at["url"] = urllib.parse.urljoin(base, available_at["url"])
    return variant


def main() -> int:
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    upstream_source = sys.argv[2] if len(sys.argv) > 2 else UPSTREAM_DEFAULT

    target = root / METADATA_REL
    current = json.loads(target.read_text())
    upstream = load(upstream_source)

    hosted = {
        variant["name"]: variant
        for variant in current["variants"]
        if variant["name"].startswith(HOSTED_PREFIXES)
    }

    merged = []
    for variant in upstream["variants"]:
        merged.append(hosted.get(variant["name"]) or absolutize(variant, upstream_source))

    dropped = [v["name"] for v in current["variants"] if v["name"] not in {
        m["name"] for m in merged
    }]
    merged_names = {v["name"] for v in merged}
    upstream_names = {v["name"] for v in upstream["variants"]}
    if not merged_names.issuperset(upstream_names):
        missing = sorted(upstream_names - merged_names)
        print(f"error: upstream variants missing from merge: {missing}", file=sys.stderr)
        return 1

    current["variants"] = merged
    target.write_text(json.dumps(current, indent=2) + "\n")
    print(
        f"merged {len(merged)} variants into {target} "
        f"(hosted: {sorted(hosted)}, dropped: {dropped or 'none'})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
