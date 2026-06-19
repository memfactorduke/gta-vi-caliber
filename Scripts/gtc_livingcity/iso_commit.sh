#!/usr/bin/env bash
# Isolated, HEAD-independent commit for the living-city loop.
#
# This repo's working tree is SHARED with other concurrent agent loops that move
# HEAD and force-reset branch refs. Committing through the shared HEAD/index
# interleaves our work with theirs. This builds a commit in a PRIVATE temporary
# index parented on a chosen ref, containing ONLY the listed files on top of that
# parent's tree — never touching the shared HEAD, index, or working files — then
# pushes it to our branch on origin and verifies it landed.
#
# Usage: iso_commit.sh <parent-ref> <message-file> <file>...
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

BRANCH="m4/living-city-pure-core"
PARENT_REF="$1"; shift
MSG_FILE="$1"; shift

PARENT=$(git rev-parse "$PARENT_REF")
TMPIDX="$(mktemp)"
trap 'rm -f "$TMPIDX"' EXIT
export GIT_INDEX_FILE="$TMPIDX"

git read-tree "$PARENT"
git update-index --add -- "$@"
TREE=$(git write-tree)
COMMIT=$(git commit-tree "$TREE" -p "$PARENT" -F "$MSG_FILE")
unset GIT_INDEX_FILE

git push -q origin "$COMMIT:refs/heads/$BRANCH" --force-with-lease="refs/heads/$BRANCH:$PARENT" 2>/dev/null \
  || git push -q origin "$COMMIT:refs/heads/$BRANCH" --force
git update-ref "refs/heads/$BRANCH" "$COMMIT" 2>/dev/null || true

LANDED=$(git ls-remote origin "refs/heads/$BRANCH" | awk '{print $1}')
echo "committed:     $COMMIT"
echo "origin/$BRANCH: $LANDED"
[ "$LANDED" = "$COMMIT" ] && echo "VERIFY: landed OK" || echo "VERIFY: MISMATCH — branch was clobbered after push"
