#!/bin/bash

# ================== 用户需修改的参数 ==================
BASE_MAC="/home/tsc/g4cmp/build/Analysis1-build/pce.mac"          # 基础宏文件完整路径
EXEC="/home/tsc/g4cmp/build/Analysis1-build/RISQTutorial"      # 可执行文件
DATA_DIR="/mnt/sim/g4simbytsc/temp"                            # 输出 root 文件和图片的临时目录
ARCHIVE_PARENT="/mnt/sim/g4simbytsc/Analysis1"                 # 归档父目录

PREFIX="energy_flow_50meV_2500eV"             # 归档文件夹名前缀，实际名称为 myrun.t1, myrun.t2, ...
NBATCHES=12               # 总共跑多少批
SEED1_START=1000           # 第一个种子起始值
SEED2_START=2000           # 第二个种子起始值（每批同时加 1）

# 分析脚本名称（请确认与你的实际文件名一致）
ANALYSIS_SCRIPT="check_energy_balance_root.C"

# ========================================================

mkdir -p "$ARCHIVE_PARENT"

for ((i=0; i<NBATCHES; i++)); do
    BATCH_NUM=$((i+1))    # 轮次号从1开始
    SEED1=$((SEED1_START + i))
    SEED2=$((SEED2_START + i))

    echo "===== 轮次 ${BATCH_NUM}/${NBATCHES} | 种子: $SEED1 $SEED2 ====="

    # 创建临时宏文件：插入双种子，并移除基础宏中已有的 /random/setSeeds 行
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

    # 运行分析脚本生成图片（在移动文件之前）
    if command -v root >/dev/null 2>&1; then
        root -l -q "$ANALYSIS_SCRIPT"
    else
        echo "警告: 未找到 root 命令，跳过分析脚本。"
    fi

    # 创建本批次的归档目录，命名格式：前缀.t轮次号
    BATCH_DIR="${ARCHIVE_PARENT}/${PREFIX}.t${BATCH_NUM}"
    mkdir -p "$BATCH_DIR"

    # 移动输出文件（如果存在）
    for f in phonon_primary.root phonon_hits_active.root phonon_hits_passive.root phonon_lowenergy.root energy_balance.png; do
        if [ -f "${DATA_DIR}/${f}" ]; then
            mv "${DATA_DIR}/${f}" "$BATCH_DIR/"
        fi
    done

    # 复制基础宏文件（保留原始内容，不含种子）
    if [ -f "$BASE_MAC" ]; then
        cp "$BASE_MAC" "$BATCH_DIR/"
    fi

    # 在归档目录中写入种子信息
    echo "seed1 = $SEED1" > "${BATCH_DIR}/seed.txt"
    echo "seed2 = $SEED2" >> "${BATCH_DIR}/seed.txt"

    # 清理临时宏
    rm -f "$TEMP_MAC"

    echo "轮次 ${BATCH_NUM} 完成，归档到 $BATCH_DIR"
    echo ""
done

echo "全部批次完成！"