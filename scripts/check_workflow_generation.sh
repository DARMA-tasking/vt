#!/usr/bin/env bash

path_to_vt=${1}

cd "$path_to_vt" || exit 1

# Check workflow for Azure CI
./scripts/generate-workflow.pl ./scripts/workflow-azure-template.yml ./scripts/workflows-azure.ini

# Check workflow for Spack package job
./scripts/generate-workflow.pl ./scripts/spack-workflow-template.yml ./scripts/spack-workflow-azure.ini

result=$(git diff --name-only)

if [ -n "$result" ]; then
    echo -e "Following files need regeneration!\n"
    echo "$result"
    exit 1
else
    echo "All workflows correct"
fi
