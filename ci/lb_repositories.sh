#!/usr/bin/env bash

clone_lb_repositories() {
    local source_dir=$1
    local vt_branch=${GITHUB_BASE_REF:-${GIT_BRANCH:-${GITHUB_REF_NAME:-}}}
    if test -z "$vt_branch"
    then
        vt_branch=$(git -C "$source_dir" symbolic-ref --quiet --short HEAD 2>/dev/null || true)
    fi

    local lb_rev=${LB_REV:-develop}
    case "$vt_branch" in
        main|master) lb_rev=${LB_REV:-master} ;;
    esac

    local lb_dir="$source_dir/lib/LB"
    local comm_dir="$source_dir/lib/comm"
    if test ! -f "$lb_dir/CMakeLists.txt"
    then
        git clone -b "$lb_rev" --depth 1 https://github.com/DARMA-tasking/LB.git "$lb_dir"
    fi
    if test ! -f "$lb_dir/src/vt-lb/comm/vt/comm_vt.h" && test ! -f "$comm_dir/CMakeLists.txt"
    then
        git clone -b "${COMM_REV:-develop}" --depth 1 https://github.com/DARMA-tasking/comm.git "$comm_dir"
    fi
}
