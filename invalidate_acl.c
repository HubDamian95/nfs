// SPDX-License-Identifier: GPL-2.0
/*
 * invalidate_acl - drop the NFS client's cached ACL for one file.
 *
 * Loading the module with target_file=<path on an NFS mount> calls the NFS
 * client's own nfs_zap_acl_cache() for that file's inode. The next ACL read
 * (getxattr system.nfs4_acl, nfs4_getfacl, ...) then has to ask the server
 * instead of answering from the cache. Unload it with rmmod when done.
 *
 * nfs_zap_acl_cache() is EXPORT_SYMBOL_GPL in nfs.ko. It is declared in
 * fs/nfs/internal.h, which kernel headers do not install, so it is declared
 * here. Checked against Linux v6.8.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/magic.h>
#include <linux/nfs_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hubdamian95");
MODULE_DESCRIPTION("Invalidate the NFSv4 ACL cache for a given file");
MODULE_VERSION("0.2");

extern void nfs_zap_acl_cache(struct inode *inode);

static char *target_file;
module_param(target_file, charp, 0444);
MODULE_PARM_DESC(target_file, "Path of a file on an NFSv4 mount whose ACL cache to flush");

static int __init nfs_acl_flush_init(void)
{
	struct path path;
	struct inode *inode;
	int err;

	if (!target_file) {
		pr_err("nfs_acl_flush: no target_file specified\n");
		return -EINVAL;
	}

	err = kern_path(target_file, LOOKUP_FOLLOW, &path);
	if (err) {
		pr_err("nfs_acl_flush: cannot resolve %s (%d)\n", target_file, err);
		return err;
	}

	inode = d_inode(path.dentry);

	/*
	 * NFS_PROTO() reads NFS-specific data from the inode, so make sure
	 * this really is an NFS inode before touching it.
	 */
	if (inode->i_sb->s_magic != NFS_SUPER_MAGIC) {
		pr_err("nfs_acl_flush: %s is not on an NFS mount\n", target_file);
		err = -EINVAL;
		goto out;
	}

	if (NFS_PROTO(inode)->version != 4) {
		pr_err("nfs_acl_flush: %s is not on an NFSv4 mount\n", target_file);
		err = -EINVAL;
		goto out;
	}

	nfs_zap_acl_cache(inode);
	pr_info("nfs_acl_flush: ACL cache invalidated for %s\n", target_file);

out:
	path_put(&path);
	return err;
}

static void __exit nfs_acl_flush_exit(void)
{
	pr_info("nfs_acl_flush: module unloaded\n");
}

module_init(nfs_acl_flush_init);
module_exit(nfs_acl_flush_exit);
