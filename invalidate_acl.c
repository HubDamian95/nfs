#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/nfs_fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hubdamian95");
MODULE_DESCRIPTION("Invalidate NFSv4 ACL cache for a given file");
MODULE_VERSION("0.1");

/* Module parameter: path to the file to flush ACL */
static char *target_file = NULL;
module_param(target_file, charp, 0644);
MODULE_PARM_DESC(target_file, "File path to flush NFSv4 ACL cache");

static int __init nfs_acl_flush_init(void)
{
    struct path path;
    struct inode *inode;

    if (!target_file) {
        pr_err("nfs_acl_flush: No target_file specified\n");
        return -EINVAL;
    }

    pr_info("nfs_acl_flush: flushing ACL cache for %s\n", target_file);

    if (kern_path(target_file, LOOKUP_FOLLOW, &path) != 0) {
        pr_err("nfs_acl_flush: Failed to resolve path %s\n", target_file);
        return -ENOENT;
    }

    inode = path.dentry->d_inode;
    if (!inode) {
        pr_err("nfs_acl_flush: Got NULL inode for %s\n", target_file);
        path_put(&path);
        return -ENOENT;
    }

#ifdef CONFIG_NFS_V4
    /* Reset cached ACL for this inode */
    nfs_inode_reset_acl(inode);
    pr_info("nfs_acl_flush: ACL cache invalidated for %s\n", target_file);
#else
    pr_err("nfs_acl_flush: NFSv4 not compiled in this kernel\n");
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

