#!/usr/bin/env bash
# Check if there were any dockerfiles modified in the latest pull request.
# Note: this is specific to the way Azure checks out git repo.

diff_latest() {
  git diff --name-only "$(git log --skip=1 -1 --merges --pretty=format:%H)"..
}

if diff_latest | grep -i dockerfile > /dev/null
then
    echo "build --pull ubuntu-cpp-clean"
else
    echo "pull --ignore-pull-failures ubuntu-cpp-clean"
fi
