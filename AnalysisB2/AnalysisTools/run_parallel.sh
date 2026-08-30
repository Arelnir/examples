#!/bin/bash

# ================== 用户需修改的参数 ==================
BASE_MAC="/home/tsc/g4cmp/build/AnalysisB2-build/pce.mac"          # 基础宏文件完整路径
EXEC="/home/tsc/g4cmp/build/AnalysisB2-build/RISQTutorial"      # 可执行文件
WORK_BASE="/mnt/sim/g4simbytsc/temp_resampling"                  # 工作目录基名，实际使用 temp1,temp2,temp3
ARCHIVE_PARENT="/mnt/sim/g4simbytsc/AnalysisB2/resampling600_1keV"                 # 归档父目录

PREFIX="posscan"          # 归档文件夹名前缀
N_X=30                    # X方向点数
N_Y=10                    # Y方向点数
N_Z=2                     # Z方向点数

# X坐标范围 (mm)
X_MIN=-4.999
X_MAX=4.999
# Y坐标范围 (mm)
Y_MIN=-1.666
Y_MAX=1.666
# Z坐标值 (mm)，顺序与标签对应
Z_COORDS="-0.200 0.200"

# 种子起始值
SEED1_START=1000
SEED2_START=2000

# 最大并行进程数
MAX_PARALLEL=3

# ========================================================

# 禁止 core dump
ulimit -c 0

# 创建归档父目录和工作目录
mkdir -p "$ARCHIVE_PARENT"
for i in $(seq 1 $MAX_PARALLEL); do
    mkdir -p "${WORK_BASE}${i}"
done

# 日志与失败记录
LOGFILE="${ARCHIVE_PARENT}/run_parallel.log"
FAILFILE="${ARCHIVE_PARENT}/failed_positions.txt"
> "$LOGFILE"
> "$FAILFILE"

# 将终端输出同时写入日志
exec > >(tee -a "$LOGFILE") 2>&1

# 总位置数
TOTAL_POSITIONS=$((N_X * N_Y * N_Z))
echo "总位置数：$TOTAL_POSITIONS"

# 计算步长
X_STEP=$(awk -v min="$X_MIN" -v max="$X_MAX" -v n="$N_X" 'BEGIN { printf "%.10f", (max-min)/(n-1) }')
Y_STEP=$(awk -v min="$Y_MIN" -v max="$Y_MAX" -v n="$N_Y" 'BEGIN { printf "%.10f", (max-min)/(n-1) }')

# 当前正在运行的后台任务数
running=0

# 任务函数
run_task() {
    local x_label=$1
    local y_label=$2
    local z_label=$3
    local index=$4
    local seed1=$5
    local seed2=$6
    local x_mm=$7
    local y_mm=$8
    local z_mm=$9
    local workdir=${10}

    # 转换为 cm（宏使用 cm）
    local x_cm=$(awk -v val="$x_mm" 'BEGIN { printf "%.6f", val/10.0 }')
    local y_cm=$(awk -v val="$y_mm" 'BEGIN { printf "%.6f", val/10.0 }')
    local z_cm=$(awk -v val="$z_mm" 'BEGIN { printf "%.6f", val/10.0 }')

    echo "开始位置 ${index}/${TOTAL_POSITIONS} | 标签($x_label,$y_label,$z_label) | X=${x_mm} mm Y=${y_mm} mm Z=${z_mm} mm | 种子: $seed1 $seed2"

    # 创建临时宏
    local temp_mac=$(mktemp /tmp/pos_macro.XXXXXX.mac)
    {
        echo "/random/setSeeds $seed1 $seed2"
        echo "/gps/pos/centre $x_cm $y_cm $z_cm cm"
        grep -v -e '/random/setSeeds' -e '/gps/pos/centre' "$BASE_MAC"
    } > "$temp_mac"

    # 进入独立工作目录
    cd "$workdir" || { echo "无法进入 $workdir"; rm -f "$temp_mac"; return 1; }
    rm -f "$workdir"/*.root

    "$EXEC" "$temp_mac"
    local exit_code=$?

    # 创建归档目录，使用整数标签命名
    local batch_dir="${ARCHIVE_PARENT}/${PREFIX}.${x_label}.${y_label}.${z_label}"
    mkdir -p "$batch_dir"

    # 移动输出文件
    for f in phonon_primary.root phonon_hits.root; do
        if [ -f "${workdir}/${f}" ]; then
            mv "${workdir}/${f}" "$batch_dir/"
        fi
    done

    # 复制宏文件（不含种子和位置行）
    grep -v -e '/random/setSeeds' -e '/gps/pos/centre' "$BASE_MAC" > "${batch_dir}/pce.mac"

    # 记录种子和坐标
    echo "seed1 = $seed1" > "${batch_dir}/seed.txt"
    echo "seed2 = $seed2" >> "${batch_dir}/seed.txt"
    echo "x_mm = $x_mm" >> "${batch_dir}/position.txt"
    echo "y_mm = $y_mm" >> "${batch_dir}/position.txt"
    echo "z_mm = $z_mm" >> "${batch_dir}/position.txt"

    if [ $exit_code -ne 0 ]; then
        echo "警告：位置 ${index} 失败，退出码 $exit_code"
        echo "${PREFIX}.${x_label}.${y_label}.${z_label}" >> "$FAILFILE"
    else
        echo "位置 ${index} 完成，归档到 $batch_dir"
    fi

    rm -f "$temp_mac"
}

# 主循环：Z -> Y -> X
index=0
for ((iz=0; iz<N_Z; iz++)); do
    # 获取 Z 坐标和标签
    if [ $iz -eq 0 ]; then
        Z="-0.200"
        Z_LABEL=-1
    else
        Z="0.200"
        Z_LABEL=1
    fi

    for ((iy=0; iy<N_Y; iy++)); do
        Y=$(awk -v min="$Y_MIN" -v step="$Y_STEP" -v i="$iy" 'BEGIN { printf "%.6f", min + i*step }')
        # 生成 Y 标签：-5 到 +5 不含 0
        if [ $iy -lt $((N_Y/2)) ]; then
            Y_LABEL=$((iy - N_Y/2))
        else
            Y_LABEL=$((iy - N_Y/2 + 1))
        fi

        for ((ix=0; ix<N_X; ix++)); do
            X=$(awk -v min="$X_MIN" -v step="$X_STEP" -v i="$ix" 'BEGIN { printf "%.6f", min + i*step }')
            # 生成 X 标签：-15 到 +15 不含 0
            if [ $ix -lt $((N_X/2)) ]; then
                X_LABEL=$((ix - N_X/2))
            else
                X_LABEL=$((ix - N_X/2 + 1))
            fi

            index=$((index+1))
            seed1=$((SEED1_START + index))
            seed2=$((SEED2_START + index))

            # 轮转选择工作目录
            workdir="${WORK_BASE}$(( (index-1) % MAX_PARALLEL + 1 ))"

            # 启动后台任务
            run_task "$X_LABEL" "$Y_LABEL" "$Z_LABEL" "$index" "$seed1" "$seed2" "$X" "$Y" "$Z" "$workdir" &

            # 控制最大并行数
            ((running++))
            if ((running >= MAX_PARALLEL)); then
                wait -n
                ((running--))
            fi
        done
    done
done

# 等待所有剩余后台任务完成
wait

echo "全部位置扫描完成！"
echo "日志文件：$LOGFILE"
echo "失败位置列表：$FAILFILE"