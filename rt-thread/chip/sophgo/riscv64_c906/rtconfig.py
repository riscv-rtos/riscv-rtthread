import os
import environ

# toolchains options
ARCH        ='risc-v'
VENDOR      ='t-head'
CPU         ='c906_smode'
CROSS_TOOL  ='gcc'

if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')
else:
    RTT_ROOT = r'../../..'

if os.getenv('RTT_CC'):
    CROSS_TOOL = os.getenv('RTT_CC')

if  CROSS_TOOL == 'gcc':
    PLATFORM    = 'gcc'
    # EXEC_PATH   = '123'
    EXEC_PATH   = r'/opt/Xuantie-900-gcc-elf-newlib-x86_64-V2.8.1/bin'
else:
    print('Please make sure your toolchains is GNU GCC!')
    exit(0)

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

BUILD = 'debug'

if PLATFORM == 'gcc':
    # toolchains
    PREFIX  = os.getenv('RTT_CC_PREFIX') or 'riscv64-unknown-elf-'
    CC      = PREFIX + 'gcc'
    CXX     = PREFIX + 'g++'
    AS      = PREFIX + 'gcc'
    AR      = PREFIX + 'ar'
    LINK    = PREFIX + 'gcc'
    TARGET_EXT = 'elf'
    SIZE    = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY  = PREFIX + 'objcopy'

    DEVICE  = ' -mcmodel=medany -march=rv64imafdc -mabi=lp64'
    CFLAGS  = DEVICE + ' -Wno-cpp -fvar-tracking -ffreestanding -fno-common -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -D_POSIX_SOURCE '
    AFLAGS  = ' -c' + DEVICE + ' -x assembler-with-cpp -D__ASSEMBLY__'

    TARGET_BOARD = os.path.join(os.getenv('RV_RTT_ROOT'), 'boards', os.getenv('CHIP_FAMILY'), os.getenv('BOARD_NAME'))

    LINKER_SCRIPTS = 'link.lds'
    LINKER_SCRIPTS_PATH = ' -L ' + os.path.join(TARGET_BOARD, 'scripts', 'c906')

    LFLAGS  = DEVICE + ' -nostartfiles -Wl,--gc-sections,-Map=rtthread.map,-cref,-u,_start -T ' + LINKER_SCRIPTS + LINKER_SCRIPTS_PATH + ' -lsupc++ -lgcc -static'
    CPATH   = ''
    LPATH   = ''

    if BUILD == 'debug':
        CFLAGS += ' -O0 -ggdb'
        AFLAGS += ' -ggdb'
    else:
        CFLAGS += ' -O2 -Os'

    CXXFLAGS = CFLAGS

python_cmd, version = environ.GetPythonInfo()
if None == python_cmd or None == version:
    print("Python version is not supported, please use python3!")
    exit(0)

combine_cmd = os.path.join(os.getenv('RV_RTT_ROOT'), 'boards', os.getenv('CHIP_FAMILY'))
combine_cmd = python_cmd + ' ' + os.path.join(combine_cmd, 'combine.py')

board = os.getenv('BOARD_NAME')
if board == 'milkv_duo256m_spinor' or board == 'milkv_duo_spinor':
    combine_type = ' --type spinor'
else:
    combine_type = ' --type sd'

combine_board = ' --board ' + board
combine_chip = ' --chip ' + os.getenv('CHIP_NAME')
combine_path = ' --file_path ' + os.path.join(os.getcwd())
combine_file = ' --file_name rtthread.bin'

DUMP_ACTION = OBJDUMP + ' -D -S $TARGET > rtthread.asm\n'
POST_ACTION = OBJCPY + ' -O binary $TARGET rtthread.bin \n' + SIZE + ' $TARGET \n'
POST_ACTION += combine_cmd + combine_type + combine_board + combine_chip + combine_path + combine_file +  ' \n'
