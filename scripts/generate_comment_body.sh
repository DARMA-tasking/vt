#!/usr/bin/env bash
set -euo pipefail

# Args
compilation_errors_warnings_out="$1"
cmake_output_log="$2"
bake_target="$3"
repository_name="$4"
run_id="$5"
job_status="$6"
run_attempt="$7"

newline=$'\n'
max_comment_size=2000

[[ "$job_status" == "success" || "$job_status" == "failure" ]] && succeeded=1 || succeeded=0

warnings_errors=$(<"$compilation_errors_warnings_out")
tests_failures=""
if [[ -f "$cmake_output_log" ]]; then
    tests_failures=$(sed -n '/The following tests FAILED:/,$p' "$cmake_output_log")
    tests_failures=${tests_failures//$'\t'/  }        # convert tabs to two spaces
fi

if (( succeeded )); then
    [[ -z "$warnings_errors" ]] && warnings_errors='Compilation – successful'
    [[ -z "$tests_failures"  ]] && tests_failures='Testing – passed'
else
    [[ -z "$warnings_errors" && -z "$tests_failures" ]] && \
        warnings_errors='Build failed for unknown reason. Check build logs'
fi

# ----- Combine, truncate if needed -----
val="${warnings_errors}${newline}${newline}${tests_failures}"
if (( ${#val} > max_comment_size )); then
    val="${val:0:max_comment_size}${newline}${newline} ==> And there is more. Read log. <=="
fi

build_link=$(
  gh api "repos/${repository_name}/actions/runs/${run_id}/attempts/${run_attempt}/jobs" |
  jq -r --arg target "$bake_target" '.jobs | map(select(.name | contains($target))) | .[0].html_url'
)

commit_date=$(date -u -d "$(gh api "repos/${GITHUB_REPOSITORY}/commits/${GITHUB_SHA}" --jq '.commit.committer.date')" '+%Y-%m-%d %H:%M:%S %Z')

# ----- Final comment body -----
comment_body=$(cat <<EOF
Build for $GITHUB_SHA ($commit_date)

\`\`\`
$val
\`\`\`

[View job log]($build_link)
EOF
)

printf '%s\n' "$comment_body"
