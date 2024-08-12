#!/usr/bin/env bash

path_to_vt=${1}

cd "$path_to_vt" || exit 1

./scripts/generate-workflow.pl ./scripts/workflow-azure-template.yml ./scripts/workflows-azure.ini

result=$(git diff --name-only)

if [ -n "$result" ]; then
    echo -e "Following files need regeneration!\n"
    echo "$result"
    exit 1
else
    echo "All workflows correct"
fi
