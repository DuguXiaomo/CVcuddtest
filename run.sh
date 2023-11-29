#!/bin/bash

# 默认参数值
INPUT_FILE=""
OUTPUT_FILE=""
SEED=""
COUNT=""

# 处理命令行参数
while [[ $# -gt 0 ]]; do
    case "$1" in
        -input)
            INPUT_FILE=$2
            shift
            shift
            ;;
        -output)
            OUTPUT_FILE=$2
            shift
            shift
            ;;
        -seed)
            SEED=$2
            shift
            shift
            ;;
        -count)
            COUNT=$2
            shift
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# 检查参数是否完整
if [[ -z "$INPUT_FILE" || -z "$OUTPUT_FILE" || -z "$SEED" || -z "$COUNT" ]]; then
    echo "Usage: run.sh -input <constraint json file> -output <result json file> -seed <num> -count <num>"
    exit 1
fi

# 运行程序
./bdd_processing "$INPUT_FILE" "$OUTPUT_FILE" "$SEED" "$COUNT"