#!/usr/bin/env python3

import os
import sys
import json
import platform
import urllib.request
from urllib.parse import urlparse
import tarfile
import argparse
import hashlib
import socket
import time
import shutil

RISCV_TOOLS_PATH_DEFAULT = os.path.join(os.path.expanduser('~'), '.riscv-tools')
DOWNLOAD_TIMEOUT = 20
MAX_RETRIES = 3

is_China_ip = None

def need_using_mirror_download():
    global is_China_ip

    if is_China_ip != None:
        return is_China_ip

    if not os.path.exists(RISCV_TOOLS_PATH_DEFAULT):    
        os.makedirs(RISCV_TOOLS_PATH_DEFAULT, exist_ok=True)

    config_file = os.path.join(RISCV_TOOLS_PATH_DEFAULT, 'config.json')
    if os.path.exists(config_file):
        config = json.load(config_file)
        if config['country'] == "China":
            return True
        elif config['country'] == "Default":
            return False

    data = {}
    try:
        ip_url = 'https://ifconfig.me/ip'
        response = urllib.request.urlopen(ip_url, timeout=5)
        ip = response.read().decode('utf-8').strip()

        url = 'http://www.ip-api.com/json/' + ip
        with urllib.request.urlopen(url, timeout=5) as response:
            data = response.read().decode('utf-8')
            json_data = json.loads(data)
            if json_data['country'] == 'China':
                is_China_ip = True
            else:
                is_China_ip = False

            server_decision = "auto decision based on IP location"
    except:
        if (-time.timezone)/3600 == 8:
            is_China_ip = True
        else:
            is_China_ip = False
        server_decision = "auto decision based on timezone"

    print("[Use {} server - {}]".format(("China" if is_China_ip else "Foreign"), server_decision))

    if is_China_ip == True:
        data = {"country": "China"}
    else:
        data = {"country": "Default"}

    with open(config_file, 'w') as file:
        json.dump(data, file, indent=4)
        
    return is_China_ip

def download_progress(block_num, block_size, total_size):
    read_so_far = block_num * block_size
    if total_size > 0:
        percent = read_so_far * 1e2 / total_size
        s = "\r%5.1f%% %*d / %d" % (
            percent, len(str(total_size)), read_so_far, total_size)
        sys.stdout.write(s)
        if read_so_far >= total_size: # close the line after the download finishes
            sys.stdout.write('\n')
    else: # total size is unknown
        sys.stdout.write("read %d\n" % (read_so_far,))

def download(url, toolchain_download_path):
    socket.setdefaulttimeout(DOWNLOAD_TIMEOUT)    # socker timeout 
    retry_count = 0
    
    while retry_count < MAX_RETRIES:
        try:
            urllib.request.urlretrieve(url, toolchain_download_path, reporthook=download_progress)
            return True
        
        except urllib.error.URLError as e:
            print("\nError downloading the tar file:", e)
            retry_count += 1
        
        except Exception as e:
            print("\nError downloading the tar file:", e)
            retry_count += 1
        
    return False

def check(toolchain_download_path, size, sha256):
    if os.path.getsize(toolchain_download_path) != size:
        print(f"size check error {size}", os.path.getsize(toolchain_download_path))
        return False

    if hashlib.sha256(open(toolchain_download_path, 'rb').read()).hexdigest() != sha256:
        print(f"sha256 check error {sha256}", hashlib.sha256(open(toolchain_download_path, 'rb').read()).hexdigest())
        return False
    
    return True

def download_tool(url_dict, tools_path, tool_dir, size_dict, sha256_dict):

    if need_using_mirror_download():
        print("Using China mirror download")
        url = url_dict['cn']
        size = size_dict['cn']
        sha256 = sha256_dict['cn']
    else:
        url = url_dict['default']
        size = size_dict['default']
        sha256 = sha256_dict['default']


    parsed_url = urlparse(url)
    file_name = parsed_url.path.split('/')[-1]
    
    tools_download_path = os.path.join(tools_path, "dist")
    tools_extra_path = os.path.join(tools_path, "tools")

    if not os.path.exists(tools_download_path):    
        os.makedirs(tools_download_path, exist_ok=True)

    dist_path = os.path.join(tools_download_path, file_name)
    print(f"Downloading: {url}")
    print("File path:", dist_path)

    if not os.path.exists(dist_path):
        if not download(url, dist_path):
            print(f"Download {file_name} failed.")
            return False

    if not check(dist_path, size, sha256):
        print(f"{file_name} check failed. downloading again...")

        os.remove(dist_path)

        if not download(url, dist_path):
            print(f"Download {file_name} again failed.")
            os.remove(dist_path)
            return False
        
        if not check(dist_path, size, sha256):
            print(f"{file_name} check failed again.")
            os.remove(dist_path)
            return False
    
    print(f"Extracting {file_name}")
    if not os.path.exists(tools_extra_path):
        os.makedirs(tools_extra_path, exist_ok=True)

    with tarfile.open(dist_path, "r") as tar:
        tar.extractall(tools_extra_path)

    entries = os.listdir(tools_extra_path)
    for entry in entries:
        if os.path.isdir(os.path.join(tools_extra_path, entry)):

            if entry != tool_dir:
                src = os.path.join(tools_extra_path, entry)
                dst = os.path.join(tools_extra_path, tool_dir)
                try:
                    os.rename(src, dst)
                except FileNotFoundError:
                    print(f"The folder '{src}' does not exist.")
                except Exception as e:
                    print(f"Error occurred: {e}")


    print("Downloaded and extracted successfully.")

    return True

def start_download(versions, tool_dir, export_path, default_path, sys_platform):
    if os.path.exists(export_path):
        return True
    else:
        for version_info in versions:
            for platform_info, details in version_info.items():
                urls = details['urls']
                # system check
                if sys_platform == platform_info:

                    # get mirror url
                    url_dict = {}
                    size_dict = {}
                    sha256_dict = {}

                    for item in urls:
                        url_dict[item['region']] = item['url']
                        size_dict[item['region']] = item['size']
                        sha256_dict[item['region']] = item['sha256']

                    if url_dict == None or size_dict == None or sha256_dict == None:
                        print("No url or size or sha256 found, please check.")
                        return False
                    
                    # start download
                    if not download_tool(url_dict, default_path, tool_dir, size_dict, sha256_dict):
                        print(f"Failed to download: {url_dict}")        
                        break
                    
                    if os.path.exists(export_path):
                        return True
                    else:
                        print(f"Download {url_dict} failed, please download it again.")
                        break
        
    return False

def install(json_path):
    sys_platform = f'{platform.system()}-{platform.machine()}'.lower()
    system_tup = ('linux', 'darwin', 'windows')

    if platform.system() not in system_tup :
        win_system = platform.system().split('-')[0]
        if win_system == 'MINGW64_NT' or win_system == 'MINGW32_NT' or win_system == 'CYGWIN_NT':
            sys_platform = 'windows-amd64'

    print("Platform:", sys_platform)

    if os.getenv('RISCV_TOOLS_PATH'):
        default_path = os.getenv('RISCV_TOOLS_PATH')
    else:
        default_path = RISCV_TOOLS_PATH_DEFAULT
        
    # json isn't exist, use default toolchain path
    if not os.path.exists(json_path):
        return False

    with open(json_path, 'r') as json_file:
        tools_json = json.load(json_file)

    if tools_json['version'] != "1.0.0":
        print("unsupport version")
        return False
    
    result = False

    for tool in tools_json['tools']:
        attribute = tool['attribute']
        description = tool['description']
        tool_name = tool['tool_name']
        versions = tool['version']
        export_paths = tool['export_paths']
        
        tool_export_path = os.path.join(default_path, 'tools',)
        for item in export_paths:
            tool_export_path = os.path.join(tool_export_path, item)
        
        tool_dir = export_paths[0]

        if (attribute == 'toolchain'):
            # print("Installing toolchain: ", description)
            os.environ['RTT_EXEC_PATH'] = tool_export_path
            os.environ['RTT_CC_PREFIX'] = tool_name
            result = start_download(versions, tool_dir, tool_export_path, default_path, sys_platform)
        
        elif (attribute == 'build_dependency'):
            # print("Installing build dependency: ", description)
            start_download(versions, tool_dir, tool_export_path, default_path, sys_platform)
            result = True
        
        else:
            print(f"Unsupport attribute: {attribute}")
            result = False

    return result

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Install tools')
    parser.add_argument('--json', type=str, required=True, help='Path to the JSON file')
    args = parser.parse_args()

    if install(args.json):
        print("Tools installed successfully.")
    else:
        print("Failed to install tools.")
        
    sys.exit(0)
    