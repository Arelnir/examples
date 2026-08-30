#!/bin/bash

# ================== 用户需修改的参数 ==================
BASE_MAC="/home/tsc/g4cmp/examples/Analysis1/pce.mac"          # 基础宏文件完整路径
EXEC="/home/tsc/g4cmp/build/Analysis1-build/RISQTutorial"      # 可执行文件路径
DATA_DIR="/mnt/sim/g4simbytsc/temp"                            # 模拟输出 root 文件的临时目录
ARCHIVE_PARENT="/mnt/sim/g4simbytsc/Analysis1"                 # 归档父目录

PREFIX="myrun"                # 归档文件夹名前缀（实际文件夹名：myrun.t1, myrun.t2 ...）
NBATCHES=10                   # 总批次数
SEED1_START=1000              # 第一个种子起始值
SEED2_START=2000              # 第二个种子起始值（每批同时加 1）

# 分析脚本路径（与当前脚本同目录）
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ANALYSIS_SCRIPT="${SCRIPT_DIR}/analyze_main.C"

# 汇总目录：${ARCHIVE_PARENT}/${PREFIX}_summary
SUMMARY_DIR="${ARCHIVE_PARENT}/${PREFIX}_summary"

# ========================================================

# 创建必要的目录
mkdir -p "$ARCHIVE_PARENT" "$SUMMARY_DIR"

# ------------------ 批处理循环 ------------------
for ((i=0; i<NBATCHES; i++)); do
    BATCH_NUM=$((i+1))                     # 轮次号从 1 开始
    SEED1=$((SEED1_START + i))
    SEED2=$((SEED2_START + i))

    echo "===== 轮次 ${BATCH_NUM}/${NBATCHES} | 种子: $SEED1 $SEED2 ====="

    # 创建临时宏：插入双种子，并删除基础宏中已有的种子行
    TEMP_MAC=$(mktemp /tmp/batch_macro.XXXXXX.mac)
    echo "/random/setSeeds $SEED1 $SEED2" > "$TEMP_MAC"
    sed '/\/random\/setSeeds/d' "$BASE_MAC" >> "$TEMP_MAC"

    # 运行模拟
    "$EXEC" "$TEMP_MAC"
    if [ $? -ne 0 ]; then
        echo "模拟失败，中止。"
        rm -f "$TEMP_MAC"
        exit 1
    fi

    # 创建归档目录（myrun.t1, myrun.t2 ...）
    BATCH_DIR="${ARCHIVE_PARENT}/${PREFIX}.t${BATCH_NUM}"
    mkdir -p "$BATCH_DIR"

    # 移动模拟输出（主分支只输出 primary 和 hits 两个 root 文件）
    for f in phonon_primary.root phonon_hits.root; do
        if [ -f "${DATA_DIR}/${f}" ]; then
            mv "${DATA_DIR}/${f}" "$BATCH_DIR/"
        fi
    done

    # 复制基础宏文件（不含种子）
    if [ -f "$BASE_MAC" ]; then
        cp "$BASE_MAC" "$BATCH_DIR/"
    fi

    # 记录种子信息
    echo "seed1 = $SEED1" > "${BATCH_DIR}/seed.txt"
    echo "seed2 = $SEED2" >> "${BATCH_DIR}/seed.txt"

    # 清理临时宏
    rm -f "$TEMP_MAC"

    echo "轮次 ${BATCH_NUM} 完成，归档到 $BATCH_DIR"
    echo ""
done

# ------------------ 合并所有批次 ------------------
echo "===== 开始合并所有批次的 ROOT 文件 ====="

# 清空汇总目录中的旧文件
rm -f "$SUMMARY_DIR/phonon_primary.root" "$SUMMARY_DIR/phonon_hits.root"

# 收集所有归档子目录
subdirs=( ${ARCHIVE_PARENT}/${PREFIX}.t* )
if [ ${#subdirs[@]} -eq 0 ]; then
    echo "错误：没有找到以 ${PREFIX}.t 开头的归档文件夹。"
    exit 1
fi

primary_files=()
hits_files=()
for dir in "${subdirs[@]}"; do
    if [ -f "$dir/phonon_primary.root" ]; then
        primary_files+=("$dir/phonon_primary.root")
    fi
    if [ -f "$dir/phonon_hits.root" ]; then
        hits_files+=("$dir/phonon_hits.root")
    fi
done

if [ ${#primary_files[@]} -eq 0 ] || [ ${#hits_files[@]} -eq 0 ]; then
    echo "错误：归档文件夹中缺少 phonon_primary.root 或 phonon_hits.root。"
    exit 1
fi

echo "合并 ${#primary_files[@]} 个 primary 文件..."
hadd -f "$SUMMARY_DIR/phonon_primary.root" "${primary_files[@]}"

echo "合并 ${#hits_files[@]} 个 hits 文件..."
hadd -f "$SUMMARY_DIR/phonon_hits.root" "${hits_files[@]}"

# ------------------ 复制分析脚本并运行 ------------------
echo "===== 复制分析脚本到汇总目录 ====="
cp "$ANALYSIS_SCRIPT" "$SUMMARY_DIR/"

echo "===== 在汇总目录中运行 ROOT 分析 ====="
cd "$SUMMARY_DIR"
root -l -q analyze_main.C

echo ""
echo "全部完成！模拟、归档、合并、分析均已执行。"
echo "结果保存在: $SUMMARY_DIR"