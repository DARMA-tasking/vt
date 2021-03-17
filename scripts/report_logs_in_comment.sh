#!/usr/bin/env bash

# What this script does?
# * extracts warnings and errors from compilation log file,
# * extracts failed tests from tests' log file (if any)
# * if there aren't any errors or warnings - informs about it,
# * if all tests passed - informs about it,
# * puts a comment with both reports in PR thread on GitHub
#
#
# Why max_comment_size=64450 ?
#
# Maximum length of the comment body is 65536 characters.
# https://github.community/t/maximum-length-for-the-comment-body-in-issues-and-pr/148867
#
# Minus some decorations, comment's title, and additional description gives about 64.5k characters.
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
#    "$(System.JobId)"

compilation_errors_warnings_out="$1"
cmake_output_log="$2"
build_number="$3"
pull_request_number="$4"
repository_name="$5"
github_pat="$6"
build_id="$7"
job_id="$8"
task_id="28db5144-7e5d-5c90-2820-8676d630d9d2"

# Extract compilation's errors and warnings from log file
warnings_errors=$(cat "$compilation_errors_warnings_out")
if test -z "$warnings_errors"
then
    warnings_errors='Compilation - successful'
fi

# Extract tests' report from log file
delimiter="-=-=-=-"
tests_failures=""
if test -f "$cmake_output_log"
then
    tests_failures=$(< "$cmake_output_log" sed -n -e '/The following tests FAILED:/,$p')
    tests_failures=${tests_failures//$'\n'/$delimiter}
    tabulation="  "
    tests_failures=${tests_failures//$'\t'/$tabulation}
    if test -z "$tests_failures"
    then
        tests_failures='Testing - passed'
    fi
fi

# Concatenate both reports into one
val="$warnings_errors""$delimiter""$delimiter""$tests_failures"
max_comment_size=64450
if test ${#val} -gt "$max_comment_size"
then
    val="${val:0:max_comment_size}%0D%0A%0D%0A%0D%0A ==> And there is more. Read log. <=="
fi

# Build comment
commit_sha="$(git log --skip=1 -1  --pretty=format:%H)"
build_link='https://dev.azure.com/DARMA-tasking/DARMA/_build/results?buildId='"$build_id"'&view=logs&j='"$job_id"'&t='"$task_id"
comment_body="Build for $commit_sha\n\n"'```'"\n$val\n"'```'"\n\nBuild log: $build_link"

# Fix new lines
new_line="\n"
comment_body=${comment_body//$delimiter/$new_line}
quotation_mark="\""
new_quotation_mark="\\\""
comment_body=${comment_body//$quotation_mark/$new_quotation_mark}

# Ensure there's no temporary json file
if test -f data.json
then
    rm data.json
fi

# Prepare data send with request to GitHub
# TODO (STRZ) - remove commit_sha
{
echo "{"
echo '  "event_type": "comment-pr",'
echo '  "client_payload": {'
echo '    "commit_sha": "'"$(git log --skip=1 -1  --pretty=format:%H)"'",'
echo '    "comment_title": "'"$build_number"'",'
echo '    "comment_content": "'"$comment_body"'",'
echo '    "pr_number": "'"$pull_request_number"'"'
echo "  }"
echo "}"
} >> data.json

# Send GitHub request to post a PR comment
curl                                                                    \
    --request POST                                                      \
    --url https://api.github.com/repos/"$repository_name"/dispatches    \
    --header "Accept: application/vnd.github.everest-preview+json"      \
    --header "Authorization: token $github_pat"                         \
    --data "@data.json"

# Clean up
rm data.json
