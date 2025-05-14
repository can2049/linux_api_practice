#!/bin/bash

cd "$(dirname "$0")" || exit 1

# set clang-format style file
# STYLE_FILE="/path/to/.clang-format"

clang_format=$(:-$(which clang-format) || which clang-format-12 || which clang-format-11 || which clang-format-10)
if [ -z "$clang_format" ]; then
    echo "clang_format not found, please install it first."
    exit 1
fi

# 格式化所有.cpp和.h文件
find . -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.cc" | while read file; do
    echo "format: $file"
    # use clang-format style file
    # clang-format -i -style=file:"$STYLE_FILE" "$file"
    $clang_format -i "$file"
done

echo "done"
