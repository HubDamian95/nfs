// nfs_acl_flush.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/nfs_fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hubertd");
MODULE_DESCRIPTION("Invalidate NFSv4 ACL cache for a given file");
MODULE_VERSION("0.1");

/* Module parameter: path to the file to flush ACL */
static char *file_path = NULL;
module_param(file_path, charp, 0644);
MODULE_PARM_DESC(file_path, "File path to flush NFSv4 ACL cache");

static int __init nfs_acl_flush_init(void)
{
    struct path path;
    struct inode *inode;

    if (!file_path) {
        pr_err("No file_path specified\n");
        return -EINVAL;
    }

    pr_info("nfs_acl_flush: flushing ACL cache for %s\n", file_path);

    if (kern_path(file_path, LOOKUP_FOLLOW, &path) != 0) {
        pr_err("Failed to resolve path %s\n", file_path);
        return -ENOENT;
    }

    inode = path.dentry->d_inode;
    if (!inode) {
        pr_err("Got NULL inode for %s\n", file_path);
        path_put(&path);
        return -ENOENT;
    }

    /* Invalidate cached ACL */
#ifdef CONFIG_NFS_V4
    nfs_inode_reset_acl(inode);
    pr_info("ACL cache invalidated for %s\n", file_path);
#else
    pr_err("NFSv4 not compiled in this kernel\n");
#endif

    path_put(&path);
    return 0;
}

static void __exit nfs_acl_flush_exit(void)
{
    pr_info("nfs_acl_flush module unloaded\n");
}

module_init(nfs_acl_flush_init);
module_exit(nfs_acl_flush_exit);
