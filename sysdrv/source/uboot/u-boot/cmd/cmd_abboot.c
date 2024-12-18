// cmd_mycmd.c
#include <common.h>
#include <command.h>

// 命令处理函数
static int do_abboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    if (argc < 2) {
        printf("Usage: mycmd <arg>\n");
        return CMD_RET_USAGE;
    }

    printf("Hello, U-Boot! Argument: %s\n", argv[1]);
    return 0; // 成功返回 0
}

// 注册命令
U_BOOT_CMD(
    abboot,             // 命令名称
    2,                 // 最大参数数量
    1,                 // 是否允许在启动脚本中执行（1 表示允许）
    do_abboot,          // 处理函数
    "My custom command",  // 简短描述
    "mycmd <arg>\nCustom command example."  // 使用说明
);