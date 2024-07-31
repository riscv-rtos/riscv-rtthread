#!/usr/bin/env python

import os
import re
import subprocess
import sys
import platform
import shutil
import argparse

python_version = None 
python_cmd = None

def _read_env_file(file_path):
    env_vars = {}
    with open(file_path, 'r') as file:
        for line in file:
            # Removes the newline character at the end of the line
            line = line.strip()
            
            # Removes leading and trailing whitespace
            if not line or line.startswith('#'):
                continue

            # Split rows into variable names and values
            parts = line.split('=', 1)
            if len(parts) == 2:
                name, value = parts
                env_vars[name] = value
    return env_vars

def _get_command_name_version(versions):
    command_name = None
    command_version = None

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
                    command_version = match.group(1)
                    if python3_pattern.match(command_version):
                        command_name = cmd
                        print(f"{command_name} {command_version}")
                        break
        except subprocess.CalledProcessError:
            continue
        except FileNotFoundError:
            continue

    return command_version, command_name

def _get_combine_fip_arguments(board_name, pre_build_path, image_file):
    file_path = os.path.join(pre_build_path, 'fsbl', 'build', board_name, 'blmacros.env')

    env_variables = _read_env_file(file_path)

    MONITOR_RUNADDR = env_variables.get('MONITOR_RUNADDR')
    BLCP_2ND_RUNADDR = env_variables.get('BLCP_2ND_RUNADDR')
    
    print("MONITOR_RUNADDR:", MONITOR_RUNADDR)
    print("BLCP_2ND_RUNADDR:", BLCP_2ND_RUNADDR)

    CHIP_CONF_PATH = os.path.join(pre_build_path, 'fsbl', 'build', board_name, 'chip_conf.bin')

    BLCP_IMG_RUNADDR = '0x05200200'
    BLCP_PARAM_LOADADDR = '0'
    NAND_INFO = '00000000'
    NOR_INFO = 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
    FIP_COMPRESS = "lzma"

    BLCP_PATH = os.path.join(pre_build_path, 'fsbl', 'test', 'empty.bin')
    DDR_PARAM_TEST_PATH = os.path.join(pre_build_path, 'fsbl', 'test', 'cv181x', 'ddr_param.bin')

    MONITOR_PATH = os.path.join(pre_build_path, 'opensbi', board_name, 'fw_dynamic.bin')
    LOADER_2ND_PATH = os.path.join(pre_build_path, 'u-boot-2021.10', board_name, 'u-boot-raw.bin')
    BL2 = os.path.join(pre_build_path, 'fsbl', 'build', board_name, 'bl2.bin')
    arguments = [
        '-v', 'genfip',
        'fip.bin',
        '--MONITOR_RUNADDR=' + MONITOR_RUNADDR,
        '--BLCP_2ND_RUNADDR=' + BLCP_2ND_RUNADDR,
        '--CHIP_CONF=' + CHIP_CONF_PATH,
        '--NOR_INFO=' + NOR_INFO,
        '--NAND_INFO=' + NAND_INFO,
        '--BL2=' + BL2,
        '--BLCP_IMG_RUNADDR=' + BLCP_IMG_RUNADDR,
        '--BLCP_PARAM_LOADADDR=' + BLCP_PARAM_LOADADDR,
        '--BLCP=' + BLCP_PATH,
        '--DDR_PARAM=' + DDR_PARAM_TEST_PATH,
        '--BLCP_2ND=' + image_file,
        '--MONITOR=' + MONITOR_PATH,
        '--LOADER_2ND=' + LOADER_2ND_PATH,
        '--compress=' + FIP_COMPRESS
    ]
    return arguments

def do_combine_fip(board_name, soc_name, image_path, image_file):

    output_path = os.path.join(image_path, 'output', board_name)
    if not os.path.exists(output_path):
        os.makedirs(output_path, exist_ok=True)

    pre_build_path = os.path.normpath(os.path.join(os.getcwd(), '../pre-build'))

    arguments = _get_combine_fip_arguments(board_name, pre_build_path, os.path.join(image_path, image_file))
    
    tool_path = os.path.join(pre_build_path, 'fsbl', 'plat', soc_name, 'fiptool.py')
    command = [python_cmd, tool_path] + arguments
    result = subprocess.run(command, 
                            capture_output=False, 
                            text=True,
                            check=False)

    return_code = result.returncode
    if return_code != 0:
        print(f"Error: {result.stderr}")
        return False
    
    output_file = os.path.join(output_path, 'fip.bin')
    shutil.copy('fip.bin', output_file)

    os.remove('fip.bin')
    return True

def do_mkimage(board_name, image_path, image_file, output_name):
    run_flag = True

    output_path = os.path.join(image_path, 'output', board_name)
    if not os.path.exists(output_path):
        os.makedirs(output_path, exist_ok=True)

    if (platform.system() == 'Windows'):
        user_home_dir = os.path.expanduser('~')
        tools_path = os.path.join(user_home_dir, '.riscv-tools', 'tools', 'msys64', 'usr', 'bin')
        print(tools_path)
        if 'PATH' in os.environ:
            os.environ['PATH'] += os.pathsep + tools_path
        else:
            os.environ['PATH'] = tools_path

    run_cmd = 'lzma'

    with open(os.path.join(image_path, image_file), 'rb') as input_file:
        image_data = input_file.read()

    try:
        with open('Image.lzma', 'wb') as output_file:
            result = subprocess.run([run_cmd, '-c', '-9', '-f', '-k'], 
                                    input=image_data, 
                                    stdout=output_file, 
                                    check=True)
    except subprocess.CalledProcessError as e:
        print(f"lzma run error, errorcode: {e.returncode}")
        run_flag = False
    except FileNotFoundError:
        print("lzma not found, please check ...")
        run_flag = False
    except Exception as e:
        print(f"errorcode: {e}")
        run_flag = False

    if run_flag == False:
        return False

    input_file = os.path.join(os.getcwd(), 'multi.its')
    output_file = os.path.join(output_path, output_name)
    run_cmd = 'mkimage'
    try:
        result = subprocess.run([run_cmd, '-f', input_file, '-r', output_file], 
                                capture_output=False,
                                text=True, 
                                check=True)

    except subprocess.CalledProcessError as e:
        print(f"mkimage run error, errorcode: {e.returncode}")
        run_flag = False
    except FileNotFoundError:
        print("mkimage not found, please check ...")
        run_flag = False
    except Exception as e:
        print(f"errorcode: {e}")
        run_flag = False

    os.remove('Image.lzma')
    
    return run_flag

def do_combine_image(board_name, image_path, image_file, file_type):

    print("start compress kernel...")
    
    output_name = 'boot.' + file_type

    if not do_mkimage(board_name, image_path, image_file, output_name):
        return False
    
    if file_type == 'spinor' or file_type == 'spinand':
        pre_build_path = os.path.normpath(os.path.join(os.getcwd(), '../../pre-build'))
        xml_path = os.path.join(pre_build_path, 'boards', board_name, 'partition', 'partition_spinor.xml')
        tool_path = os.path.join(pre_build_path, 'tools', 'common', 'image_tool', 'raw2cimg.py')

        output_path = os.path.join(image_path, 'output', board_name)
        output_file = os.path.join(output_path, output_name)

        command = [python_cmd, tool_path, output_file, output_path, xml_path]
        result = subprocess.run(command, 
                                capture_output=False, 
                                text=True,
                                check=False)

        return_code = result.returncode
        if return_code != 0:
            print(f"Error: {result.stderr}")
            return False

    print("compress kernel done...")
    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='combine fip or boot.sd or boot.spinor')
    parser.add_argument('--type', type=str, required=True, help='Combine image type')
    parser.add_argument('--board', type=str, required=True, help='Combine target board')
    parser.add_argument('--chip', type=str, required=True, help='Combine target chip')
    parser.add_argument('--file_path', type=str, required=True, help='Path to the image file')
    parser.add_argument('--file_name', type=str, required=True, help='Image file name')
    args = parser.parse_args()
    
    script_path = os.path.abspath(__file__)
    script_dir = os.path.dirname(script_path)
    
    board_name = args.board

    if args.chip == 'riscv64_c906_little':
        soc_name = 'cv180x'
    elif args.chip == 'riscv64_c906':
        soc_name = 'cv181x'
    else:
        print("invalid chip")
        sys.exit(1)

    python_version, python_cmd = _get_command_name_version(('python3', 'python'))
    if python_version == None or python_cmd == None:
        print(f"Please install python3.8 or later version!")
        sys.exit(1)

    if args.type == 'sd' or args.type == 'spinor' or args.type == 'spinand':
        os.chdir(os.path.join(script_dir, board_name, 'dtb'))
        result = do_combine_image(board_name, args.file_path, args.file_name, args.type)
    elif args.type == 'fip':
        os.chdir(os.path.join(script_dir, board_name))
        result = do_combine_fip(board_name, soc_name, args.file_path, args.file_name)
    else:
        print("invalid type")
        sys.exit(1)

    if not result:
        print("Combine image failed.")
        sys.exit(1)

    sys.exit(0)