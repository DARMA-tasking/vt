#!/usr/bin/env bash
# Check if there were any dockerfiles modified in the PR.
# Note: this is specific to Azure - in the pipeline's local branch git history
# contains one merge commit for all of the changes in the PR.

diff_latest() {
  printf "Files modified in the pull request:\n" >&2
  git diff --name-only HEAD^1 HEAD | tee >(cat >&2)
}

# Decide which docker-compose command to use:
if diff_latest | grep -i dockerfile > /dev/null
then
    echo "build --pull"
else
    echo "pull --ignore-pull-failures"
fi
