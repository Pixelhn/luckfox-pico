#include <common.h>
#include <command.h>
#include <ubi_uboot.h>

// 命令处理函数
static int do_abboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	struct ubi_volume_desc *ubi_volume = NULL;
    int ret;
	ulong addr = simple_strtoul(argv[2], NULL, 16);;
    
    if (argc < 2)
    {
        printf("Usage: mycmd <arg> %d\n", argc);
        return CMD_RET_USAGE;
    }

	if (ubi_part(argv[1], NULL))
    {
		printf("ubi_part %s error!\n", argv[1]);
		return CMD_RET_FAILURE;
	}

    ubi_volume = ubi_open_volume_nm(0, "kernel", UBI_READWRITE);
	if (IS_ERR(ubi_volume))
    {
		printf("cannot open \"%s\", error %d", "kernel", (int)PTR_ERR(ubi_volume));
		return CMD_RET_FAILURE;
	}

    printf("read to volume: %lX\n", addr);
    ret = ubi_volume_read("kernel", (char *)addr, 4*1024*1024);
    if (ret < 0)
    {
        printf("Failed to read UBI volume: %d %lX\n", ret, addr);
        ubi_close_volume(ubi_volume);
        return CMD_RET_FAILURE;
    }

    printf("success!\n");
	ubi_close_volume(ubi_volume);

    return 0; // 成功返回 0
}

// 注册命令
U_BOOT_CMD(
    abboot,             // 命令名称
    3,                 // 最大参数数量
    1,                 // 是否允许在启动脚本中执行（1 表示允许）
    do_abboot,          // 处理函数
    "",  // 简短描述
    ""  // 使用说明
);