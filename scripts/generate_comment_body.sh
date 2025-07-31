#!/usr/bin/env bash

# What this script does?
# * extracts warnings and errors from compilation log file,
# * extracts failed tests from tests' log file (if any)
# * if there aren't any errors or warnings - informs about it,
# * if all tests passed - informs about it,
#
#
# Why max_comment_size=2000 ?
#
# Maximum length of the comment body is 65536 characters.
# https://github.community/t/maximum-length-for-the-comment-body-in-issues-and-pr/148867
#
# Minus some decorations, comment's title, and additional description gives about 64.5k characters.
# There are 21 pipelines, so 64.5k/21 gives about 3k characters per pipeline.
#
#
# Example of usage:
# ./generate_comment_body.sh                           \
#    "$(build_root)/vt/compilation_errors_warnings.out" \
#    "$(build_root)/vt/cmake-output.log"                \
#    "$(Build.BuildNumber)"                             \
#    "$(System.PullRequest.PullRequestNumber)"          \
#    "$(Build.Repository.Name)"                         \
#    "$GITHUB_PAT"                                      \
#    "$(Build.BuildId"                                  \
#    "$(System.JobId)"                                  \
#    "$(Agent.JobStatus)"

compilation_errors_warnings_out="$1"
cmake_output_log="$2"
bake_target="$3"
repository_name="$4"
run_id="$5"
job_status="$6"
commit_sha="$7"
run_attempt="$8"

echo "job_status: $job_status"
if [[ "$job_status" == "success" || "$job_status" == "failure" ]]; then
    succeeded=1
else
    succeeded=0
fi

warnings_errors=$(cat "$compilation_errors_warnings_out")

delimiter="-=-=-=-"
tests_failures=""
if test -f "$cmake_output_log"
then
    tests_failures=$(< "$cmake_output_log" sed -n -e '/The following tests FAILED:/,$p')
    tests_failures=${tests_failures//$'\n'/$delimiter}
    tabulation="  "
    tests_failures=${tests_failures//$'\t'/$tabulation}
fi

if test "$succeeded" -eq 1
then
    if test -z "$warnings_errors"
    then
        warnings_errors='Compilation - successful'
    fi

    if test -z "$tests_failures"
    then
        tests_failures='Testing - passed'
    fi
else
    if test -z "$warnings_errors" && test -z "$tests_failures"
    then
        warnings_errors='Build failed for unknown reason. Check build logs'
    fi
fi

# Concatenate both reports into one
val="$warnings_errors""$delimiter""$delimiter""$tests_failures"
max_comment_size=2000
if test ${#val} -gt "$max_comment_size"
then
    val="${val:0:max_comment_size}%0D%0A%0D%0A%0D%0A ==> And there is more. Read log. <=="
fi

build_link=$(
  gh api "repos/${repository_name}/actions/runs/${run_id}/attempts/${run_attempt}/jobs" |
  jq -r --arg target "$bake_target" '.jobs | map(select(.name | contains($target))) | .[0].html_url'
)

# Build comment
commit_date=$(date -u -d "$(gh api "repos/${GITHUB_REPOSITORY}/commits/${GITHUB_SHA}" --jq '.commit.committer.date')" '+%Y-%m-%d %H:%M:%S %Z')
comment_body="Build for $commit_sha ($commit_date)\n\n"'```'"\n$val\n"'```'"\n\n[View job log]($build_link)"

# Fix new lines
new_line="\n"
comment_body=${comment_body//$delimiter/$new_line}
quotation_mark="\""
new_quotation_mark="\\\""
comment_body=${comment_body//$quotation_mark/$new_quotation_mark}

printf '%s\n' "$comment_body"
