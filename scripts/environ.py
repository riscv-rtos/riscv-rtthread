#!/usr/bin/env python3

import os
import sys
import shutil
import subprocess
import re

def GetConfigInfo(file_path, config_key):
    info_value = None

    with open(file_path, 'r') as file:
        for line in file:
            line = line.strip()
            if line.startswith('#') or '= is not set' in line:
                continue

            config_name = 'CONFIG_' + config_key + '_'
            if config_name in line and '=y' in line:
                name_key, name_value = line.split('=', 1)
                info_value = name_key.split(config_name)[1].strip().lower()

            if info_value != None:
                return info_value

    return None

def GetBoardInfo(file_path):
    if not os.path.exists(file_path):
        CHIP_FAMILY = 'sophgo'
        BOARD_NAME = 'milkv_duo256m_sd'
        CHIP_NAME = 'riscv64_c906'
    else: 
        CHIP_FAMILY = GetConfigInfo(file_path, 'CHIP_FAMILY')
        BOARD_NAME = GetConfigInfo(file_path, 'BOARD_NAME')
        CHIP_NAME = GetConfigInfo(file_path, 'CHIP_NAME')
    
    os.environ['CHIP_FAMILY'] = CHIP_FAMILY
    os.environ['BOARD_NAME'] = BOARD_NAME
    os.environ['CHIP_NAME'] = CHIP_NAME

    TARGET_CHIP = os.path.join(os.getenv('RTT_ROOT'), 'chip', CHIP_FAMILY, CHIP_NAME)

    return TARGET_CHIP

def GetPythonInfo():
    versions=('python3', 'python')
    python_version = None
    python_cmd = None

    for cmd in versions:
        version_pattern = re.compile(r'\b(\d+\.\d+(\.\d+)?)\b')     # version pattern
        python3_pattern = re.compile(r'^3\.\d+\.\d+$')              # 3.x.x pattern

        try:
            result = subprocess.run([cmd, '--version'],
                                    capture_output=True,
                                    text=True,
                                    check=False)
            
            if result.returncode == 0:
                match = version_pattern.search(result.stdout)
                if match:
                    python_version = match.group(1)
                    if python3_pattern.match(python_version):
                        python_cmd = cmd
                        break
        except subprocess.CalledProcessError:
            continue
        except FileNotFoundError:
            continue

    return python_cmd, python_version, 
