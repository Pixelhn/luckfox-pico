/*
 *  nandwrite.c
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *		  2003 Thomas Gleixner (tglx@linutronix.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Overview:
 *   This utility writes a binary image directly to a NAND flash
 *   chip or NAND chips contained in DoC devices. This is the
 *   "inverse operation" of nanddump.
 *
 * tglx: Major rewrite to handle bad blocks, write data with or without ECC
 *	 write oob data only on request
 *
 * Bug/ToDo:
 */

#define PROGRAM_NAME "nandwrite"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>

#include <asm/types.h>
#include "mtd/mtd-user.h"
#include "common.h"
#include <libmtd.h>

#include "mtd_utils_all.h"

static void display_help(int status)
{
	fprintf(status == EXIT_SUCCESS ? stdout : stderr,
"Usage: nandwrite [OPTION] MTD_DEVICE [INPUTFILE|-]\n"
"Writes to the specified MTD device.\n"
"\n"
"  -a, --autoplace         Use auto OOB layout\n"
"  -k, --skip-all-ffs      Skip pages that contain only 0xff bytes\n"
"  -m, --markbad           Mark blocks bad if write fails\n"
"  -n, --noecc             Write without ecc\n"
"  -N, --noskipbad         Write without bad block skipping\n"
"  -o, --oob               Input contains oob data\n"
"  -O, --onlyoob           Input contains oob data and only write the oob part\n"
"  -s addr, --start=addr   Set output start address (default is 0)\n"
"  --skip-bad-blocks-to-start                          Skip bad blocks when seeking to the start address\n"
"  -p, --pad               Pad writes to page size\n"
"  -b, --blockalign=1|2|4  Set multiple of eraseblocks to align to\n"
"      --input-skip=length Skip |length| bytes of the input file\n"
"      --input-size=length Only read |length| bytes of the input file\n"
"  -q, --quiet             Don't display progress messages\n"
"  -h, --help              Display this help and exit\n"
"  -V, --version           Output version information and exit\n"
	);
	exit(status);
}

static void display_version(void)
{
	common_print_version();
	printf("Copyright (C) 2003 Thomas Gleixner\n"
			"\n"
			"%1$s comes with NO WARRANTY\n"
			"to the extent permitted by law.\n"
			"\n"
			"You may redistribute copies of %1$s\n"
			"under the terms of the GNU General Public Licence.\n"
			"See the file `COPYING' for more information.\n",
			PROGRAM_NAME);
	exit(EXIT_SUCCESS);
}

static const char	*standard_input = "-";
static const char	*mtd_device, *img;
static long long	mtdoffset;
static long long	inputskip;
static long long	inputsize;
static bool		quiet;
static bool		writeoob;
static bool		onlyoob;
static bool		markbad;
static bool		noecc;
static bool		autoplace;
static bool		skipallffs;
static bool		noskipbad;
static bool		pad;
static bool		skip_bad_blocks_to_start;
static int		blockalign = 1; /* default to using actual block size */

static void process_options_spinand(void)
{
	pad = true;
}

static void erase_buffer(void *buffer, size_t size)
{
	const uint8_t kEraseByte = 0xff;

	if (buffer != NULL && size > 0)
		memset(buffer, kEraseByte, size);
}

static int is_virt_block_bad(struct mtd_dev_info *mtd, int fd,
				long long offset)
{
	int i, ret = 0;

	for (i = 0; i < blockalign; ++i) {
		ret = mtd_is_bad(mtd, fd, offset / mtd->eb_size + i);
		if (ret)
			break;
	}

	return ret;
}

int flash_write_buf(char *mtd_dev, char *input_buf, size_t start, size_t len)
{
	int fd = -1;
	int ifd = -1;
	int pagelen;
	long long blockstart = -1;
	struct mtd_dev_info mtd;
	int ret;
	bool failed = true;
	/* contains all the data read from the file so far for the current eraseblock */
	unsigned char *filebuf = NULL;
	size_t filebuf_max = 0;
	size_t filebuf_len = 0;
	/* points to the current page inside filebuf */
	unsigned char *writebuf = NULL;
	/* points to the OOB for the current page in filebuf */
	unsigned char *oobbuf = NULL;
	libmtd_t mtd_desc;
	int ebsize_aligned;
	uint8_t write_mode;
	size_t all_ffs_cnt = 0;

	ret = flash_erase(mtd_dev, start, len);
	if (ret) {
		printf("%s flash_erase failed, ret=%d\n", __func__, ret);
		return ret;
	}

	mtd_device = mtd_dev;
	mtdoffset = start;
	process_options_spinand();

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1)
		sys_errmsg_die("%s", mtd_device);

	mtd_desc = libmtd_open();
	if (!mtd_desc)
		errmsg_die("can't initialize libmtd");

	/* Fill in MTD device capability structure */
	if (mtd_get_dev_info(mtd_desc, mtd_device, &mtd) < 0)
		errmsg_die("mtd_get_dev_info failed");

	/*
	 * Pretend erasesize is specified number of blocks - to match jffs2
	 *   (virtual) block size
	 * Use this value throughout unless otherwise necessary
	 */
	ebsize_aligned = mtd.eb_size * blockalign;

	if (mtdoffset & (mtd.min_io_size - 1))
		errmsg_die("The start address is not page-aligned !\n"
			   "The pagesize of this NAND Flash is 0x%x.\n",
			   mtd.min_io_size);

	/* Select OOB write mode */
	if (noecc)
		write_mode = MTD_OPS_RAW;
	else if (autoplace)
		write_mode = MTD_OPS_AUTO_OOB;
	else
		write_mode = MTD_OPS_PLACE_OOB;

	if (noecc)  {
		ret = ioctl(fd, MTDFILEMODE, MTD_FILE_MODE_RAW);
		if (ret) {
			switch (errno) {
			case ENOTTY:
				errmsg_die("ioctl MTDFILEMODE is missing");
			default:
				sys_errmsg_die("MTDFILEMODE");
			}
		}
	}

	/* Determine if we are reading from standard input or from a file. */
	pagelen = mtd.min_io_size + ((writeoob) ? mtd.oob_size : 0);

	/* Check, if file is page-aligned */
	if (!pad && (len % pagelen) != 0) {
		fprintf(stderr, "Input file is not page-aligned. Use the padding option.\n");
		goto closeall;
	}

	/* Skip bad blocks on the way to the start address if necessary */
	if (skip_bad_blocks_to_start) {
		long long bbs_offset = 0;

		while (bbs_offset < mtdoffset) {
			ret = is_virt_block_bad(&mtd, fd, bbs_offset);
			if (ret < 0) {
				sys_errmsg("%s: MTD get bad block failed", mtd_device);
				goto closeall;
			} else if (ret == 1) {
				if (!quiet)
					fprintf(stderr, "Bad block at %llx, %u block(s) from %llx will be skipped\n",
						bbs_offset, blockalign, bbs_offset);
				mtdoffset += ebsize_aligned;
			}
			bbs_offset += ebsize_aligned;
		}
	}

	/* Check, if len fits into device */
	if ((len / pagelen) * mtd.min_io_size > mtd.size - mtdoffset) {
		fprintf(stderr, "Image %lld bytes, NAND page %d bytes, OOB area %d bytes, device size %lld bytes\n",
				len, pagelen, mtd.oob_size, mtd.size);
		sys_errmsg("Input file does not fit into device");
		goto closeall;
	}

	/*
	 * Allocate a buffer big enough to contain all the data (OOB included)
	 * for one eraseblock. The order of operations here matters; if ebsize
	 * and pagelen are large enough, then "ebsize_aligned * pagelen" could
	 * overflow a 32-bit data type.
	 */
	filebuf_max = ebsize_aligned / mtd.min_io_size * pagelen;
	filebuf = xmalloc(filebuf_max);
	erase_buffer(filebuf, filebuf_max);

	/*
	 * Get data from input and write to the device while there is
	 * still input to read and we are still within the device
	 * bounds. Note that in the case of standard input, the input
	 * length is simply a quasi-boolean flag whose values are page
	 * length or zero.
	 */
	while ((len > 0 || writebuf < filebuf + filebuf_len)
		&& mtdoffset < mtd.size) {
		bool allffs;

		/*
		 * New eraseblock, check for bad block(s)
		 * Stay in the loop to be sure that, if mtdoffset changes because
		 * of a bad block, the next block that will be written to
		 * is also checked. Thus, we avoid errors if the block(s) after the
		 * skipped block(s) is also bad (number of blocks depending on
		 * the blockalign).
		 */
		while (blockstart != (mtdoffset & (~ebsize_aligned + 1))) {
			blockstart = mtdoffset & (~ebsize_aligned + 1);

			/*
			 * if writebuf == filebuf, we are rewinding so we must
			 * not reset the buffer but just replay it
			 */
			if (writebuf != filebuf) {
				erase_buffer(filebuf, filebuf_len);
				filebuf_len = 0;
				writebuf = filebuf;
			}

			if (!quiet)
				fprintf(stdout, "Writing data to block %lld at offset 0x%llx\n",
						 blockstart / ebsize_aligned, blockstart);

			if (noskipbad)
				continue;

			ret = is_virt_block_bad(&mtd, fd, blockstart);

			if (ret < 0) {
				sys_errmsg("%s: MTD get bad block failed", mtd_device);
				goto closeall;
			} else if (ret == 1) {
				if (!quiet)
					fprintf(stderr,
						"Bad block at %llx, %u block(s) will be skipped\n",
						blockstart, blockalign);

				mtdoffset = blockstart + ebsize_aligned;

				if (mtdoffset > mtd.size) {
					errmsg("too many bad blocks, cannot complete request");
					goto closeall;
				}
			}
		}

		/* Read more data from the input if there isn't enough in the buffer */
		if (writebuf + mtd.min_io_size > filebuf + filebuf_len) {
			size_t readlen = mtd.min_io_size;
			size_t alreadyread = (filebuf + filebuf_len) - writebuf;
			size_t tinycnt = alreadyread;
			ssize_t cnt = 0;

			while (tinycnt < readlen) {
				if (len < mtd.min_io_size)
					cnt = len;
				else
					cnt = readlen - tinycnt;
				memcpy(writebuf + tinycnt, input_buf, cnt);
				input_buf += cnt;

				if (cnt == 0) { /* EOF */
					break;
				} else if (cnt < 0) {
					perror("File I/O error on input");
					goto closeall;
				}
				tinycnt += cnt;
				if (len < mtd.min_io_size)
					break;
			}

			/* No padding needed - we are done */
			if (tinycnt == 0) {
				/*
				 * For standard input, set length to 0 to signal
				 * the end of the "file". For nonstandard input,
				 * leave it as-is to detect an early EOF.
				 */

				break;
			}

			/* Padding */
			if (tinycnt < readlen) {
				if (!pad) {
					fprintf(stderr, "Unexpected EOF. Expecting at least %zu more bytes. Use the padding option.\n",
							readlen - tinycnt);
					goto closeall;
				}
				erase_buffer(writebuf + tinycnt, readlen - tinycnt);
			}

			filebuf_len += readlen - alreadyread;
			len -= tinycnt - alreadyread;
		}

		if (writeoob) {
			oobbuf = writebuf + mtd.min_io_size;

			/* Read more data for the OOB from the input if there isn't enough in the buffer */
			if (oobbuf + mtd.oob_size > filebuf + filebuf_len) {
				size_t readlen = mtd.oob_size;
				size_t alreadyread = (filebuf + filebuf_len) - oobbuf;
				size_t tinycnt = alreadyread;
				ssize_t cnt;

				while (tinycnt < readlen) {
					cnt = readlen - tinycnt;
					memcpy(oobbuf + tinycnt, input_buf, cnt);
					input_buf += cnt;
					if (cnt == 0) { /* EOF */
						break;
					} else if (cnt < 0) {
						perror("File I/O error on input");
						goto closeall;
					}
					tinycnt += cnt;
				}

				if (tinycnt < readlen) {
					fprintf(stderr, "Unexpected EOF. Expecting at least %zu more bytes for OOB\n",
							readlen - tinycnt);
					goto closeall;
				}

				filebuf_len += readlen - alreadyread;
				if (ifd != STDIN_FILENO) {
					len -= tinycnt - alreadyread;
				} else if (cnt == 0) {
					/* No more bytes - we are done after writing the remaining bytes */
					len = 0;
				}
			}
		}

		ret = 0;
		allffs = buffer_check_pattern(writebuf, mtd.min_io_size, 0xff);
		if (!allffs || !skipallffs) {
			/* Write out data */
			ret = mtd_write(mtd_desc, &mtd, fd, mtdoffset / mtd.eb_size,
					mtdoffset % mtd.eb_size,
					onlyoob ? NULL : writebuf,
					onlyoob ? 0 : mtd.min_io_size,
					writeoob ? oobbuf : NULL,
					writeoob ? mtd.oob_size : 0,
					write_mode);
			if (!ret && allffs)
				all_ffs_cnt++;
		}

		if (ret) {
			if (errno != EIO) {
				sys_errmsg("%s: MTD write failure", mtd_device);
				goto closeall;
			}

			/* Must rewind to blockstart if we can */
			writebuf = filebuf;

			fprintf(stderr, "Erasing failed write from %#08llx to %#08llx\n",
				blockstart, blockstart + ebsize_aligned - 1);

			if (mtd_erase_multi(mtd_desc, &mtd, fd,
					blockstart / mtd.eb_size, blockalign)) {
				int errno_tmp = errno;

				sys_errmsg("%s: MTD Erase failure", mtd_device);
				if (errno_tmp != EIO)
					goto closeall;
			}

			if (markbad) {
				fprintf(stderr, "Marking block at %08llx bad\n",
						mtdoffset & (~mtd.eb_size + 1));
				if (mtd_mark_bad(&mtd, fd, mtdoffset / mtd.eb_size)) {
					sys_errmsg("%s: MTD Mark bad block failure", mtd_device);
					goto closeall;
				}
			}
			mtdoffset = blockstart + ebsize_aligned;

			continue;
		}

		mtdoffset += mtd.min_io_size;
		writebuf += pagelen;
	}

	failed = false;

closeall:
	if (ifd > 0 && ifd != STDIN_FILENO)
		close(ifd);
	libmtd_close(mtd_desc);
	free(filebuf);
	close(fd);

	if (failed || (ifd != STDIN_FILENO && len > 0)
		   || (writebuf < filebuf + filebuf_len))
		sys_errmsg_die("Data was only partially written due to error");

	if (all_ffs_cnt) {
		fprintf(stderr, "Written %zu blocks containing only 0xff bytes\n", all_ffs_cnt);
		fprintf(stderr, "Those block may be incorrectly treated as empty!\n");
	}

	/* Return happy */
	return EXIT_SUCCESS;
}

/*
 * Main program
 */
int nandwrite(char *mtd_dev, char *file, int type)
{
	int fd = -1;
	int ifd = -1;
	int pagelen;
	long long imglen = 0;
	long long blockstart = -1;
	struct mtd_dev_info mtd;
	int ret;
	bool failed = true;
	/* contains all the data read from the file so far for the current eraseblock */
	unsigned char *filebuf = NULL;
	size_t filebuf_max = 0;
	size_t filebuf_len = 0;
	/* points to the current page inside filebuf */
	unsigned char *writebuf = NULL;
	/* points to the OOB for the current page in filebuf */
	unsigned char *oobbuf = NULL;
	libmtd_t mtd_desc;
	int ebsize_aligned;
	uint8_t write_mode;
	size_t all_ffs_cnt = 0;

	mtd_device = mtd_dev;
	img = file;
	process_options_spinand();

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1)
		sys_errmsg_die("%s", mtd_device);

	mtd_desc = libmtd_open();
	if (!mtd_desc)
		errmsg_die("can't initialize libmtd");

	/* Fill in MTD device capability structure */
	if (mtd_get_dev_info(mtd_desc, mtd_device, &mtd) < 0)
		errmsg_die("mtd_get_dev_info failed");

	/*
	 * Pretend erasesize is specified number of blocks - to match jffs2
	 *   (virtual) block size
	 * Use this value throughout unless otherwise necessary
	 */
	ebsize_aligned = mtd.eb_size * blockalign;

	if (mtdoffset & (mtd.min_io_size - 1))
		errmsg_die("The start address is not page-aligned !\n"
			   "The pagesize of this NAND Flash is 0x%x.\n",
			   mtd.min_io_size);

	/* Select OOB write mode */
	if (noecc)
		write_mode = MTD_OPS_RAW;
	else if (autoplace)
		write_mode = MTD_OPS_AUTO_OOB;
	else
		write_mode = MTD_OPS_PLACE_OOB;

	if (noecc)  {
		ret = ioctl(fd, MTDFILEMODE, MTD_FILE_MODE_RAW);
		if (ret) {
			switch (errno) {
			case ENOTTY:
				errmsg_die("ioctl MTDFILEMODE is missing");
			default:
				sys_errmsg_die("MTDFILEMODE");
			}
		}
	}

	/* Determine if we are reading from standard input or from a file. */
	if (strcmp(img, standard_input) == 0)
		ifd = STDIN_FILENO;
	else
		ifd = open(img, O_RDONLY);

	if (ifd == -1) {
		perror(img);
		goto closeall;
	}

	pagelen = mtd.min_io_size + ((writeoob) ? mtd.oob_size : 0);

	if (ifd == STDIN_FILENO) {
		imglen = inputsize ? : pagelen;
		if (inputskip) {
			errmsg("seeking stdin not supported");
			goto closeall;
		}
	} else {
		if (!inputsize) {
			struct stat st;

			if (fstat(ifd, &st)) {
				sys_errmsg("unable to stat input image");
				goto closeall;
			}
			imglen = st.st_size - inputskip;
		} else
			imglen = inputsize;

		if (inputskip && lseek(ifd, inputskip, SEEK_CUR) == -1) {
			sys_errmsg("lseek input by %lld failed", inputskip);
			goto closeall;
		}
	}

	/* Check, if file is page-aligned */
	if (!pad && (imglen % pagelen) != 0) {
		fprintf(stderr, "Input file is not page-aligned. Use the padding option.\n");
		goto closeall;
	}

	/* Skip bad blocks on the way to the start address if necessary */
	if (skip_bad_blocks_to_start) {
		long long bbs_offset = 0;

		while (bbs_offset < mtdoffset) {
			ret = is_virt_block_bad(&mtd, fd, bbs_offset);
			if (ret < 0) {
				sys_errmsg("%s: MTD get bad block failed", mtd_device);
				goto closeall;
			} else if (ret == 1) {
				if (!quiet)
					fprintf(stderr, "Bad block at %llx, %u block(s) from %llx will be skipped\n",
						bbs_offset, blockalign, bbs_offset);
				mtdoffset += ebsize_aligned;
			}
			bbs_offset += ebsize_aligned;
		}
	}

	/* Check, if length fits into device */
	if ((imglen / pagelen) * mtd.min_io_size > mtd.size - mtdoffset) {
		fprintf(stderr, "Image %lld bytes, NAND page %d bytes, OOB area %d bytes, device size %lld bytes\n",
				imglen, pagelen, mtd.oob_size, mtd.size);
		sys_errmsg("Input file does not fit into device");
		goto closeall;
	}

	/*
	 * Allocate a buffer big enough to contain all the data (OOB included)
	 * for one eraseblock. The order of operations here matters; if ebsize
	 * and pagelen are large enough, then "ebsize_aligned * pagelen" could
	 * overflow a 32-bit data type.
	 */
	filebuf_max = ebsize_aligned / mtd.min_io_size * pagelen;
	filebuf = xmalloc(filebuf_max);
	erase_buffer(filebuf, filebuf_max);

	/*
	 * Get data from input and write to the device while there is
	 * still input to read and we are still within the device
	 * bounds. Note that in the case of standard input, the input
	 * length is simply a quasi-boolean flag whose values are page
	 * length or zero.
	 */
	while ((imglen > 0 || writebuf < filebuf + filebuf_len)
		&& mtdoffset < mtd.size) {
		bool allffs;

		/*
		 * New eraseblock, check for bad block(s)
		 * Stay in the loop to be sure that, if mtdoffset changes because
		 * of a bad block, the next block that will be written to
		 * is also checked. Thus, we avoid errors if the block(s) after the
		 * skipped block(s) is also bad (number of blocks depending on
		 * the blockalign).
		 */
		while (blockstart != (mtdoffset & (~ebsize_aligned + 1))) {
			blockstart = mtdoffset & (~ebsize_aligned + 1);

			/*
			 * if writebuf == filebuf, we are rewinding so we must
			 * not reset the buffer but just replay it
			 */
			if (writebuf != filebuf) {
				erase_buffer(filebuf, filebuf_len);
				filebuf_len = 0;
				writebuf = filebuf;
			}

			if (!quiet)
				fprintf(stdout, "Writing data to block %lld at offset 0x%llx\n",
						 blockstart / ebsize_aligned, blockstart);

			if (noskipbad)
				continue;

			ret = is_virt_block_bad(&mtd, fd, blockstart);

			if (ret < 0) {
				sys_errmsg("%s: MTD get bad block failed", mtd_device);
				goto closeall;
			} else if (ret == 1) {
				if (!quiet)
					fprintf(stderr,
						"Bad block at %llx, %u block(s) will be skipped\n",
						blockstart, blockalign);

				mtdoffset = blockstart + ebsize_aligned;

				if (mtdoffset > mtd.size) {
					errmsg("too many bad blocks, cannot complete request");
					goto closeall;
				}
			}
		}

		/* Read more data from the input if there isn't enough in the buffer */
		if (writebuf + mtd.min_io_size > filebuf + filebuf_len) {
			size_t readlen = mtd.min_io_size;
			size_t alreadyread = (filebuf + filebuf_len) - writebuf;
			size_t tinycnt = alreadyread;
			ssize_t cnt = 0;

			while (tinycnt < readlen) {
				cnt = read(ifd, writebuf + tinycnt, readlen - tinycnt);
				if (cnt == 0) { /* EOF */
					break;
				} else if (cnt < 0) {
					perror("File I/O error on input");
					goto closeall;
				}
				tinycnt += cnt;
			}

			/* No padding needed - we are done */
			if (tinycnt == 0) {
				/*
				 * For standard input, set imglen to 0 to signal
				 * the end of the "file". For nonstandard input,
				 * leave it as-is to detect an early EOF.
				 */
				if (ifd == STDIN_FILENO)
					imglen = 0;

				break;
			}

			/* Padding */
			if (tinycnt < readlen) {
				if (!pad) {
					fprintf(stderr, "Unexpected EOF. Expecting at least %zu more bytes. Use the padding option.\n",
							readlen - tinycnt);
					goto closeall;
				}
				erase_buffer(writebuf + tinycnt, readlen - tinycnt);
			}

			filebuf_len += readlen - alreadyread;
			if (ifd != STDIN_FILENO) {
				imglen -= tinycnt - alreadyread;
			} else if (cnt == 0) {
				/* No more bytes - we are done after writing the remaining bytes */
				imglen = 0;
			}
		}

		if (writeoob) {
			oobbuf = writebuf + mtd.min_io_size;

			/* Read more data for the OOB from the input if there isn't enough in the buffer */
			if (oobbuf + mtd.oob_size > filebuf + filebuf_len) {
				size_t readlen = mtd.oob_size;
				size_t alreadyread = (filebuf + filebuf_len) - oobbuf;
				size_t tinycnt = alreadyread;
				ssize_t cnt;

				while (tinycnt < readlen) {
					cnt = read(ifd, oobbuf + tinycnt, readlen - tinycnt);
					if (cnt == 0) { /* EOF */
						break;
					} else if (cnt < 0) {
						perror("File I/O error on input");
						goto closeall;
					}
					tinycnt += cnt;
				}

				if (tinycnt < readlen) {
					fprintf(stderr, "Unexpected EOF. Expecting at least %zu more bytes for OOB\n",
							readlen - tinycnt);
					goto closeall;
				}

				filebuf_len += readlen - alreadyread;
				if (ifd != STDIN_FILENO) {
					imglen -= tinycnt - alreadyread;
				} else if (cnt == 0) {
					/* No more bytes - we are done after writing the remaining bytes */
					imglen = 0;
				}
			}
		}

		ret = 0;
		allffs = buffer_check_pattern(writebuf, mtd.min_io_size, 0xff);
		if (!allffs || !skipallffs) {
			/* Write out data */
			ret = mtd_write(mtd_desc, &mtd, fd, mtdoffset / mtd.eb_size,
					mtdoffset % mtd.eb_size,
					onlyoob ? NULL : writebuf,
					onlyoob ? 0 : mtd.min_io_size,
					writeoob ? oobbuf : NULL,
					writeoob ? mtd.oob_size : 0,
					write_mode);
			if (!ret && allffs)
				all_ffs_cnt++;
		}

		if (ret) {
			if (errno != EIO) {
				sys_errmsg("%s: MTD write failure", mtd_device);
				goto closeall;
			}

			/* Must rewind to blockstart if we can */
			writebuf = filebuf;

			fprintf(stderr, "Erasing failed write from %#08llx to %#08llx\n",
				blockstart, blockstart + ebsize_aligned - 1);

			if (mtd_erase_multi(mtd_desc, &mtd, fd,
					blockstart / mtd.eb_size, blockalign)) {
				int errno_tmp = errno;

				sys_errmsg("%s: MTD Erase failure", mtd_device);
				if (errno_tmp != EIO)
					goto closeall;
			}

			if (markbad) {
				fprintf(stderr, "Marking block at %08llx bad\n",
						mtdoffset & (~mtd.eb_size + 1));
				if (mtd_mark_bad(&mtd, fd, mtdoffset / mtd.eb_size)) {
					sys_errmsg("%s: MTD Mark bad block failure", mtd_device);
					goto closeall;
				}
			}
			mtdoffset = blockstart + ebsize_aligned;

			continue;
		}
		mtdoffset += mtd.min_io_size;
		writebuf += pagelen;
	}

	failed = false;

closeall:
	if (ifd > 0 && ifd != STDIN_FILENO)
		close(ifd);
	libmtd_close(mtd_desc);
	free(filebuf);
	close(fd);

	if (failed || (ifd != STDIN_FILENO && imglen > 0)
		   || (writebuf < filebuf + filebuf_len))
		sys_errmsg_die("Data was only partially written due to error");

	if (all_ffs_cnt) {
		fprintf(stderr, "Written %zu blocks containing only 0xff bytes\n", all_ffs_cnt);
		fprintf(stderr, "Those block may be incorrectly treated as empty!\n");
	}

	/* Return happy */
	return EXIT_SUCCESS;
}

