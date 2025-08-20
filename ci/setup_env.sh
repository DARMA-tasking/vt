#!/usr/bin/env bash

set -exo pipefail

bake_target=${1}

docker buildx bake --print "$bake_target" | \
  jq -r --arg t "$bake_target" '.target[$t].args | to_entries.[] | join("=")' \
  >> .env
