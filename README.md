# NFSv4 ACL cache invalidation

Two small tools for answering one question: *is my NFSv4 client enforcing a stale ACL because it is answering from its ACL cache?*

- `invalidate_acl.c` - a kernel module that drops the client's cached ACL for one file, on demand.
- `bpftrace_nfs4_acls.bt` - a bpftrace script that shows every ACL read on the client and whether it was served from the cache or fetched from the server.

> **Status:** written against the Linux v6.8 source (`fs/nfs/nfs4proc.c`, `fs/nfs/inode.c`, `fs/nfs/nfs4trace.h`). Needs testing on a real NFSv4 mount; see "Verify".

## How the client caches ACLs

On a `system.nfs4_acl` read the client goes through `nfs4_proc_get_acl()`, which:

1. revalidates the inode's change attribute,
2. zaps the ACL cache if `NFS_INO_INVALID_ACL` is set,
3. tries `nfs4_read_cached_acl()`,
4. only on a miss calls `nfs4_get_acl_uncached()`, which asks the server and fires the `nfs4:nfs4_get_acl` tracepoint.

So a stale ACL can persist for as long as step 2 never sees the inode flagged invalid. The module forces that invalidation by calling the client's own `nfs_zap_acl_cache()` (exported by `nfs.ko` with `EXPORT_SYMBOL_GPL`), which clears the cached ACL through the protocol's `clear_acl_cache` operation.

## Build

Needs kernel headers for the running kernel and a kernel built with NFS support.

```bash
make
```

## Use

```bash
# 1. watch ACL reads (needs root)
sudo bpftrace bpftrace_nfs4_acls.bt

# 2. in another shell, read the ACL of a file on an NFSv4 mount
nfs4_getfacl /mnt/nfs/file        # first read: lookup + WIRE
nfs4_getfacl /mnt/nfs/file        # repeat:     lookup only (cached)

# 3. drop the cached ACL
sudo insmod invalidate_acl.ko target_file=/mnt/nfs/file
sudo rmmod invalidate_acl

# 4. read again
nfs4_getfacl /mnt/nfs/file        # lookup + WIRE again
```

## Output

```
nfs4_getfacl     pid=1234    lookup
nfs4_getfacl     pid=1234    WIRE    fileid=42 error=0
```

A `lookup` with no `WIRE` line after it means the ACL came from the cache.

## Verify

Not yet confirmed on hardware. When testing, check:

- `make` builds on your kernel and `insmod` resolves `nfs_zap_acl_cache`.
- The flush line appears in `dmesg`, and the next read shows `WIRE`.
- `kprobe:nfs4_proc_get_acl` attaches. It is a static function, so a kernel that inlines it will need only the tracepoint.
- The module refuses files that are not on an NFSv4 mount (`-EINVAL`).

## Notes

- The earlier version of this repo called a symbol that does not exist in mainline and decoded the ACL as a `struct posix_acl`. NFSv4 ACLs are stored as an opaque blob in `struct nfs4_cached_acl`, so the script now reports who is asking and what was fetched, not individual entries.
- The module checks the superblock magic and protocol version first, because `NFS_PROTO()` reads NFS-specific data from the inode and is unsafe on any other filesystem.
