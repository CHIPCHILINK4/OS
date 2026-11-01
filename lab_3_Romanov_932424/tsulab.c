#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <linux/version.h>
#include <linux/cpumask.h>

#define TARGET_DATE_SECONDS 1155859200ULL

static struct proc_dir_entry *proc_file;

static ssize_t procfile_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos) {
    char msg[256];
    int len;

    if (*ppos > 0) {
        return 0;
    }
    
    time64_t current_sec = ktime_get_real_seconds();

    unsigned long long seconds = (current_sec - TARGET_DATE_SECONDS);
    int cpu_count = num_online_cpus();
    
    len = snprintf(msg, sizeof(msg),"Seconds since 18.08.2006: %llu\nCPUs: %d\n", seconds, cpu_count);
    
    if (count < (size_t)len) {
        return -ENOMEM;
    }
    if (copy_to_user(buffer, msg, len)) {
        return -EFAULT;
    }
    
    *ppos += len;
    return len;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_ops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_ops = {
    .read = procfile_read,
};
#endif

static int __init tsulab_init(void) {
    pr_info("Welcome to the Tomsk State University\n");
    
    proc_file = proc_create("tsulab", 0644, NULL, &proc_file_ops);
    if (!proc_file) {
        pr_err("Не удалось создать /proc/%s\n", "tsulab");
        return -ENOMEM;
    }
    
    return 0; 
}

static void __exit tsulab_exit(void) {
    proc_remove(proc_file);
    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Romanov");
MODULE_DESCRIPTION("Lab 3");