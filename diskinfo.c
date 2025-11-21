/*
 * Copyright (c) 2025 Tobias Heider <tobhe@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#define DKTYPENAMES
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#define MEG(x)  ((x) * 1024LL * (1024 / DEV_BSIZE))
#define GIG(x)  (MEG(x) * 1024LL)

struct statfs *mntbuf;
int mntidx;
int mntsize;

static void
usage(void)
{
	fprintf(stderr,
	    "usage: disks\n"
	    "       disks dev\n");
	exit(1);
}

static char *
sysctl_disknames_get(void)
{
	size_t len;
	char *buf;
	int mib[2] = { CTL_HW, HW_DISKNAMES } ;
	int ret;

	ret = sysctl(mib, 2, NULL, &len, NULL, 0);
	if (ret == -1)
		err(1, "%s: sysctl", __func__);

	buf = malloc(len);
	if (buf == NULL)
		return NULL;

	ret = sysctl(mib, 2, buf, &len, NULL, 0);
	if (ret == -1)
		err(1, "%s: sysctl", __func__);

	return buf;
}

static void
print_partition(const char *devname, const struct disklabel *dl, int i, int human)
{
	const struct partition *pp = &dl->d_partitions[i];
	char sbuf[FMT_SCALED_STRSIZE];
	int dplen;
	char *dp;

	if (DL_GETPSIZE(pp)) {
		printf("\n|-- %s%c\t", devname, DL_PARTNUM2NAME(i));
		if (human) {
			if(fmt_scaled(DL_GETPSIZE(pp) * dl->d_secsize, sbuf) == -1)
				err(1, "%s: fmt_scaled()", __func__);
			printf("%7s", sbuf);
		} else {
			printf("%16llu", DL_GETPSIZE(pp));
		}
		printf("\t%8s", fstypenames[pp->p_fstype]);

		dplen = asprintf(&dp, "/dev/%s%c", devname, DL_PARTNUM2NAME(i));
		while (mntidx < mntsize && strncmp(mntbuf[mntidx].f_mntfromname, dp, dplen) == 0) {
			if(fmt_scaled(mntbuf[mntidx].f_blocks * mntbuf[mntidx].f_bsize, sbuf) == -1)
				err(1, "%s: fmt_scaled()", __func__);

			printf("\t%s",
			    mntbuf[mntidx].f_mntonname);
			mntidx++;
		}
	}
}

static void
print_device(char *devname, int human)
{
	struct dk_inquiry di;
	struct disklabel dl;
	char sbuf[FMT_SCALED_STRSIZE];
	int dev;
	int i;

	/* Only root is allowed to do this */
	dev = opendev(devname, O_RDONLY, OPENDEV_PART, NULL);
	if (dev == -1)
		err(1, "%s: opendev", __func__);

	if (ioctl(dev, DIOCINQ, &di) == -1)
		err(1, "%s: ioctl(DIOCINQ)", __func__);
	if (ioctl(dev, DIOCGDINFO, &dl) == -1)
		err(1, "%s: ioctl(DIOCGDINFO)", __func__);

	printf("[%s %s]: ", di.vendor, di.product);
	printf("%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx\n",
	    dl.d_uid[0], dl.d_uid[1], dl.d_uid[2], dl.d_uid[3],
	    dl.d_uid[4], dl.d_uid[5], dl.d_uid[6], dl.d_uid[7]);
	printf("%-8s\t", devname);

	if (human) {
		if(fmt_scaled(DL_GETDSIZE(&dl) * dl.d_secsize, sbuf) == -1)
			err(1, "%s: fmt_scaled()", __func__);
		printf("%7s", sbuf);
	} else {
		/* Size in blocks */
		printf("%16llu", DL_GETDSIZE(&dl));
	}

	for (i = 0; i < dl.d_npartitions; i++)
		print_partition(devname, &dl, i, human);
	printf("\n\n");
}

int
main(int argc, char *argv[])
{
	char *disks = sysctl_disknames_get();
	char *p;
	int ch, human = 0;

	while ((ch = getopt(argc, argv, "h")) != -1) {
		switch (ch) {
		case 'h':
			human = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if ((mntsize = getmntinfo(&mntbuf, MNT_WAIT)) == 0)
		err(1, "%s: getmntinfo", __func__);
	mntidx = 0;

	if (argc == 1 && argv[0] != NULL) {
		print_device(argv[0], human);
		return 0;
	}

	if (human) {
		printf("DISK     \t   SIZE\t  FSTYPE\tMOUNT\n");
	} else {
		printf("DISK     \t            SIZE\t  FSTYPE\tMOUNT\n");
	}

	/* Print all of those disks*/
	while ((p = strsep(&disks, ",")) != NULL) {

		p = strsep(&p, ":");
		if (p == NULL)
			errx(1, "%s: strsep", __func__);

		print_device(p, human);

#if 0
#endif
	}

	return 0;
}
