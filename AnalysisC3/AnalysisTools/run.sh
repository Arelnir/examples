#!/bin/bash

# ================== 用户需修改的参数 ==================
BASE_MAC="/home/tsc/g4cmp/build/Analysis1-build/pce.mac"          # 基础宏文件完整路径
EXEC="/home/tsc/g4cmp/build/Analysis1-build/RISQTutorial"      # 可执行文件
DATA_DIR="/mnt/sim/g4simbytsc/temp"                            # 模拟输出 root 文件的临时目录
ARCHIVE_PARENT="/mnt/sim/g4simbytsc/Analysis1"                 # 归档父目录

PREFIX="test"          # 归档文件夹名前缀，例如 posscan
N_X=15                    # X方向点数
N_Y=5                    # Y方向点数

# ---- 坐标范围（方案一：略微内缩，避免边界） ----
X_MIN=-4.999              # X最小值 (mm)
X_MAX=4.999               # X最大值 (mm)
Y_MIN=-1.666              # Y最小值 (mm)
Y_MAX=1.666               # Y最大值 (mm)

# 种子起始值（每个位置递增）
SEED1_START=1000
SEED2_START=2000

# 日志文件与失败记录文件
LOGFILE="${ARCHIVE_PARENT}/run.log"
FAILFILE="${ARCHIVE_PARENT}/failed_positions.txt"

# ========================================================

# 禁止生成 core 文件（防止段错误时占用大量磁盘）
ulimit -c 0

mkdir -p "$ARCHIVE_PARENT" "$DATA_DIR"

# 清空旧日志和失败记录
> "$LOGFILE"
> "$FAILFILE"

# 将所有终端输出（包括程序和 echo）同时写入日志文件
exec > >(tee -a "$LOGFILE") 2>&1

# 总位置数
TOTAL_POSITIONS=$((N_X * N_Y))

# 计算步长
X_STEP=$(awk -v min="$X_MIN" -v max="$X_MAX" -v n="$N_X" 'BEGIN { printf "%.10f", (max-min)/(n-1) }')
Y_STEP=$(awk -v min="$Y_MIN" -v max="$Y_MAX" -v n="$N_Y" 'BEGIN { printf "%.10f", (max-min)/(n-1) }')

INDEX=0
# 外层循环 Y，内层循环 X
for ((iy=0; iy<N_Y; iy++)); do
    Y=$(awk -v min="$Y_MIN" -v step="$Y_STEP" -v i="$iy" 'BEGIN { printf "%.6f", min + i*step }')
    for ((ix=0; ix<N_X; ix++)); do
        X=$(awk -v min="$X_MIN" -v step="$X_STEP" -v i="$ix" 'BEGIN { printf "%.6f", min + i*step }')
        INDEX=$((INDEX+1))

        SEED1=$((SEED1_START + INDEX))
        SEED2=$((SEED2_START + INDEX))

        # 转换为 cm（宏使用 cm）
        X_CM=$(awk -v val="$X" 'BEGIN { printf "%.6f", val/10.0 }')
        Y_CM=$(awk -v val="$Y" 'BEGIN { printf "%.6f", val/10.0 }')

        echo "===== 位置 ${INDEX}/${TOTAL_POSITIONS} | X=${X} mm  Y=${Y} mm | 种子: $SEED1 $SEED2 ====="

        # 创建临时宏文件
        TEMP_MAC=$(mktemp /tmp/pos_macro.XXXXXX.mac)
        {
            echo "/random/setSeeds $SEED1 $SEED2"
            echo "/gps/pos/centre $X_CM $Y_CM 0.0 cm"
            grep -v -e '/random/setSeeds' -e '/gps/pos/centre' "$BASE_MAC"
        } > "$TEMP_MAC"

        # 进入数据目录运行模拟
        cd "$DATA_DIR" || { echo "无法进入 $DATA_DIR"; rm -f "$TEMP_MAC"; exit 1; }
        echo "开始模拟，当前时间：$(date '+%H:%M:%S')"

        "$EXEC" "$TEMP_MAC"
        EXIT_CODE=$?

        # 创建归档目录（无论成功失败都创建，便于保存已生成文件）
        BATCH_DIR="${ARCHIVE_PARENT}/${PREFIX}.${X}.${Y}.0.0"
        mkdir -p "$BATCH_DIR"

        # 移动模拟输出（如果存在）
        for f in phonon_primary.root phonon_hits.root; do
            if [ -f "${DATA_DIR}/${f}" ]; then
                mv "${DATA_DIR}/${f}" "$BATCH_DIR/"
            fi
        done

        # 复制基础宏文件（不含种子和位置行）
        if [ -f "$BASE_MAC" ]; then
            grep -v -e '/random/setSeeds' -e '/gps/pos/centre' "$BASE_MAC" > "${BATCH_DIR}/pce.mac"
        fi

        # 记录种子和坐标
        echo "seed1 = $SEED1" > "${BATCH_DIR}/seed.txt"
        echo "seed2 = $SEED2" >> "${BATCH_DIR}/seed.txt"
        echo "x_mm = $X"       >> "${BATCH_DIR}/position.txt"
        echo "y_mm = $Y"       >> "${BATCH_DIR}/position.txt"
        echo "z_mm = 0.0"      >> "${BATCH_DIR}/position.txt"

        if [ $EXIT_CODE -ne 0 ]; then
            echo "警告：位置 ${INDEX} 模拟退出码 $EXIT_CODE，已保存已生成文件并继续。"
            echo "${PREFIX}.${X}.${Y}.0.0" >> "$FAILFILE"
        else
            echo "位置 ${INDEX} 完成，归档到 $BATCH_DIR"
        fi

        # 清理临时宏
        rm -f "$TEMP_MAC"

        echo ""
    done
done

echo "全部位置扫描完成！"
echo "日志文件：$LOGFILE"
echo "失败位置列表：$FAILFILE"