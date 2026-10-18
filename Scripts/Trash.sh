#!/bin/bash
set -e
REPOSITORY_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)
for NAME in "$@"; do
    case "$NAME" in build|_Bin|_Build|_Shaders|_OpenRHI_SDK) ;; *) echo "Unsupported generated directory: $NAME" >&2; exit 1 ;; esac
    TARGET="$REPOSITORY_ROOT/$NAME"
    if [ -e "$TARGET" ]; then
        echo "Moving to Trash: $TARGET"
        if [ "$(uname)" = Darwin ]; then
            swift - "$REPOSITORY_ROOT" "$TARGET" <<'SWIFT'
import Foundation
import Darwin
enum TrashError: Error { case unexpectedTarget, targetRemains }
let root = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true).resolvingSymlinksInPath()
let target = URL(fileURLWithPath: CommandLine.arguments[2], isDirectory: true).standardizedFileURL
do {
    guard target.deletingLastPathComponent().path == root.path,
          target.resolvingSymlinksInPath().path == target.path else { throw TrashError.unexpectedTarget }
    try FileManager.default.trashItem(at: target, resultingItemURL: nil)
    guard !FileManager.default.fileExists(atPath: target.path) else { throw TrashError.targetRemains }
} catch {
    fputs("OpenRHI: cannot move generated directory to Trash: \(error)\n", stderr)
    exit(1)
}
SWIFT
        else
            gio trash "$TARGET"
        fi
        test ! -e "$TARGET"
    fi
done
