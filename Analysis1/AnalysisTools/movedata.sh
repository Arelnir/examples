#!/bin/bash

# 目标父目录
DEST_PARENT="/mnt/sim/g4simbytsc/Analysis1"

# 提示输入新文件夹名
echo "请输入新文件夹名称:"
read FOLDER_NAME

# 创建目标文件夹（若已存在则不会报错）
mkdir -p "${DEST_PARENT}/${FOLDER_NAME}"

# 移动 ROOT 文件和图片
mv phonon_primary.root phonon_hits_active.root phonon_hits_passive.root phonon_lowenergy.root energy_balance.png "${DEST_PARENT}/${FOLDER_NAME}/"

# 复制 pce.mac（请根据你的实际路径修改，这里假设 pce.mac 就在当前目录）
cp pce.mac "${DEST_PARENT}/${FOLDER_NAME}/"

echo "完成！文件已移动到 ${DEST_PARENT}/${FOLDER_NAME}"