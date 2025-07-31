#!/usr/bin/env bash

# What this script does?
# * extracts warnings and errors from compilation log file,
# * extracts failed tests from tests' log file (if any)
# * if there aren't any errors or warnings - informs about it,
# * if all tests passed - informs about it,
# * puts a comment with both reports in PR thread on GitHub
#
#
# Why max_comment_size=3000 ?
#
# Maximum length of the comment body is 65536 characters.
# https://github.community/t/maximum-length-for-the-comment-body-in-issues-and-pr/148867
#
# Minus some decorations, comment's title, and additional description gives about 64.5k characters.
# There are 21 pipelines, so 64.5k/21 gives about 3k characters per pipeline.
#
#
# What's going on with delimiter="-=-=-=-" and strange "%0D%0A"?
#
# Azure tasks have a problem with parsing newline, so it's basically a way
# to properly move around strings with newlines in them. And "%0D%0A" is just a CRLF.
#
#
# Why '\t' is replaced by "  "?
#
# After JSON standard:
# "Whitespace is not allowed within any token, except that space is allowed in strings".
# So all tabulations need to be changed into spaces.
#
#
# Example of usage:
# ./report_logs_in_comment.sh                           \
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
build_number="$3"
pull_request_number="$4"
repository_name="$5"
github_token="$6"
build_id="$7"
job_name="$8"
job_status="$9"

echo "job_status: $job_status"
if [[ "$job_status" == "success" || "$job_status" == "failure" ]]; then
    succeeded=1
else
    succeeded=0
fi

warnings_errors=$(cat "$compilation_errors_warnings_out")

delimiter="-=-=-=-"
tests_failures=""
if [[ -f "$cmake_output_log" ]]; then
    tests_failures=$(< "$cmake_output_log" sed -n -e '/The following tests FAILED:/,$p')
    tests_failures=${tests_failures//$'\n'/$delimiter}
    tabulation="  "
    tests_failures=${tests_failures//$'\t'/$tabulation}
fi

if [[ "$succeeded" -eq 1 ]]; then
    [[ -z "$warnings_errors" ]] && warnings_errors='Compilation - successful'
    [[ -z "$tests_failures" ]] && tests_failures='Testing - passed'
else
    if [[ -z "$warnings_errors" && -z "$tests_failures" ]]; then
        warnings_errors='Build failed for unknown reason. Check build logs'
    fi
fi

val="$warnings_errors""$delimiter""$delimiter""$tests_failures"
max_comment_size=3000
if [[ ${#val} -gt "$max_comment_size" ]]; then
    val="${val:0:max_comment_size}%0D%0A%0D%0A%0D%0A ==> And there is more. Read log. <=="
fi

commit_sha="$(git log --skip=1 -1  --pretty=format:%H)"
commit_date="$(TZ=UTC0 git show -s --format=%cd --date=format-local:'%Y-%m-%d %H:%M:%S' "$commit_sha")"

# Fetch numeric job ID from GitHub API
job_id=$(curl -s -H "Authorization: token $github_token" \
  "https://api.github.com/repos/${repository_name}/actions/runs/${build_id}/jobs" | \
  jq -r --arg name "$job_name" '.jobs[] | select(.name == $name) | .id')

# Fallback to run-level link if job ID is unavailable
if [[ -n "$job_id" && "$job_id" != "null" ]]; then
    build_link="[Build log](https://github.com/${repository_name}/actions/runs/${build_id}/job/${job_id})"
else
    build_link="[Build log](https://github.com/${repository_name}/actions/runs/${build_id})"
fi

comment_body="Build for $commit_sha ($commit_date UTC)\n\n"'```'"\n$val\n"'```'"\n\n$build_link"
comment_body=${comment_body//$delimiter/$'\n'}
comment_body=${comment_body//\"/\\\"}

rm -f data.json

{
echo "{"
echo '  "event_type": "comment-pr",'
echo '  "client_payload": {'
echo '    "comment_title": "'"$build_number"'",'
echo '    "comment_content": "'"$comment_body"'",'
echo '    "pr_number": "'"$pull_request_number"'"'
echo "  }"
echo "}"
} >> data.json

curl \
  --request POST \
  --url "https://api.github.com/repos/${repository_name}/dispatches" \
  --header "Accept: application/vnd.github.everest-preview+json" \
  --header "Authorization: token ${github_token}" \
  --data "@data.json"

rm -f data.json
