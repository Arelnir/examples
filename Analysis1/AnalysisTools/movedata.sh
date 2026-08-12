#!/bin/bash

# 数据源目录（root 文件、图片所在位置）
DATA_SRC="/mnt/sim/g4simbytsc/temp"

# 目标父目录
DEST_PARENT="/mnt/sim/g4simbytsc/Analysis1"

# pce.mac 路径（与脚本同目录）
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PCE_MAC_SRC="${SCRIPT_DIR}/pce.mac"

# 提示输入新文件夹名
read -p "请输入新文件夹名称: " FOLDER_NAME

# 创建目标文件夹
mkdir -p "${DEST_PARENT}/${FOLDER_NAME}"

# 移动 root 文件和图片
mv "${DATA_SRC}/phonon_primary.root" "${DATA_SRC}/phonon_hits_active.root" \
   "${DATA_SRC}/phonon_hits_passive.root" "${DATA_SRC}/phonon_lowenergy.root" \
   "${DATA_SRC}/energy_balance.png" \
   "${DEST_PARENT}/${FOLDER_NAME}/"

# 复制 pce.mac
if [ -f "${PCE_MAC_SRC}" ]; then
    cp "${PCE_MAC_SRC}" "${DEST_PARENT}/${FOLDER_NAME}/"
fi

echo "归档完成！文件已移动到 ${DEST_PARENT}/${FOLDER_NAME}"