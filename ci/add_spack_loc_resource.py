#!/usr/bin/env python3

"""Add the loc source resource needed by a VT pull-request version."""

import re
import sys
from pathlib import Path


LOC_COMMIT = "0649b54065cfd566549e38a3a5c96f417fd513ca"


if len(sys.argv) != 3 or not sys.argv[2].strip():
    sys.exit(
        "usage: add_spack_loc_resource.py <darma-vt-package.py> <version>"
    )

package_file = Path(sys.argv[1])
version = sys.argv[2]
text = package_file.read_text()

resource = f'''    resource(
        name="loc",
        git="https://github.com/DARMA-tasking/loc.git",
        commit="{LOC_COMMIT}",
        destination="lib",
        placement="loc",
        when="@{version}",
    )
'''

if resource not in text:
    version_pattern = re.compile(
        rf'^\s*version\(["\']{re.escape(version)}["\'].*$',
        re.MULTILINE,
    )
    version_match = version_pattern.search(text)
    if version_match is None:
        sys.exit(f"version {version!r} was not found in {package_file}")

    insertion_point = text.find("\n", version_match.end()) + 1
    text = text[:insertion_point] + "\n" + resource + text[insertion_point:]
    package_file.write_text(text)
