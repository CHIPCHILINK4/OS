#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <linux/random.h>

#define PROC_NAME "tsulab"
#define MAX_WORKS 5

enum lab_work_state {
    LAB_CREATED,
    LAB_QUEUED,
    LAB_RUNNING,
    LAB_DONE,
    LAB_CANCELLED
};

struct lab_work {
    struct work_struct work;
    unsigned int id;
    unsigned int stages;
    unsigned int completed;
    enum lab_work_state state;
    struct list_head list;
};

// Глобальные переменные
static struct workqueue_struct *tsu_queue;
static DEFINE_MUTEX(works_lock);
static LIST_HEAD(works_list);
static unsigned int work_counter = 0;
static struct proc_dir_entry *proc_file;

// Обработчик работы
static void lab_work_handler(struct work_struct *work) {
    struct lab_work *item = container_of(work, struct lab_work, work);
    unsigned int stage;

    mutex_lock(&works_lock);
    item->state = LAB_RUNNING;
    mutex_unlock(&works_lock);

    pr_info("Work %u started (stages: %u)\n", item->id, item->stages);

    for (stage = 0; stage < item->stages; ++stage) {
        mutex_lock(&works_lock);
        if (item->state == LAB_CANCELLED) {
            mutex_unlock(&works_lock);
            pr_info("Work %u cancelled at stage %u\n", item->id, stage);
            return;
        }
        mutex_unlock(&works_lock);

        pr_info("Work %u: stage %u/%u\n", item->id, stage + 1, item->stages);
        msleep(1000); 

        mutex_lock(&works_lock);
        item->completed = stage + 1;
        mutex_unlock(&works_lock);
    }

    mutex_lock(&works_lock);
    item->state = LAB_DONE;
    mutex_unlock(&works_lock);

    pr_info("Work %u finished\n", item->id);
}

static int create_and_queue_work(void) {
    struct lab_work *item;
    unsigned int stages;

    item = kmalloc(sizeof(*item), GFP_KERNEL);
    if (!item)
        return -ENOMEM;

    // Случайное количество этапов от 2 до MAX_STAGES
    get_random_bytes(&stages, sizeof(stages));
    stages = (stages % 4) + 2;

    item->id = 0;
    item->stages = stages;
    item->completed = 0;
    item->state = LAB_CREATED;
    INIT_WORK(&item->work, lab_work_handler);

    mutex_lock(&works_lock);
    item->id = ++work_counter;
    item->state = LAB_QUEUED;
    list_add_tail(&item->list, &works_list);
    mutex_unlock(&works_lock);

    if (!queue_work(tsu_queue, &item->work)) {
        pr_err("Failed to queue work %u\n", item->id);
        mutex_lock(&works_lock);
        list_del(&item->list);
        mutex_unlock(&works_lock);
        kfree(item);
        return -EBUSY;
    }

    pr_info("Work %u queued with %u stages\n", item->id, stages);
    return 0;
}

// Вывод статуса работ в /proc (режим берсерка)
static int tsulab_show(struct seq_file *m, void *v) {
    struct lab_work *item;
    unsigned int queued = 0, running = 0, done = 0, cancelled = 0;

    mutex_lock(&works_lock);
    list_for_each_entry(item, &works_list, list) {
        switch (item->state) {
            case LAB_CREATED:
            case LAB_QUEUED:
                queued++;
                break;
            case LAB_RUNNING:
                running++;
                break;
            case LAB_DONE:
                done++;
                break;
            case LAB_CANCELLED:
                cancelled++;
                break;
        }

        seq_printf(m, "Work %u: state=%u stages=%u completed=%u\n",
                   item->id, item->state, item->stages, item->completed);
    }
    mutex_unlock(&works_lock);

    seq_printf(m, "\nSummary:\n");
    seq_printf(m, "Queued: %u\n", queued);
    seq_printf(m, "Running: %u\n", running);
    seq_printf(m, "Done: %u\n", done);
    seq_printf(m, "Cancelled: %u\n", cancelled);

    return 0;
}

static int tsulab_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, tsulab_show, NULL);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_ops = {
    .proc_open = tsulab_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations proc_file_ops = {
    .open = tsulab_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

static int __init tsulab_init(void) {
    unsigned int i;

    pr_info("Welcome to the Tomsk State University\n");

    tsu_queue = alloc_workqueue("tsulab_queue", WQ_UNBOUND, 2);
    if (!tsu_queue) {
        pr_err("Failed to create workqueue\n");
        return -ENOMEM;
    }

    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        pr_err("Failed to create /proc/%s\n", PROC_NAME);
        destroy_workqueue(tsu_queue);
        return -ENOMEM;
    }

    for (i = 0; i < MAX_WORKS; ++i) {
        if (create_and_queue_work() != 0) {
            pr_err("Failed to create work %u\n", i + 1);
        }
    }

    pr_info("Module loaded successfully\n");
    return 0;
}

static void __exit tsulab_exit(void) {
    struct lab_work *item, *tmp;

    pr_info("TSU Lab 4: Unloading...\n");

    if (proc_file) {
        proc_remove(proc_file);
        pr_info("/proc/%s removed\n", PROC_NAME);
    }

    flush_workqueue(tsu_queue);

    mutex_lock(&works_lock);
    list_for_each_entry_safe(item, tmp, &works_list, list) {
        pr_info("Work %u completed or cancelled (state: %u, stages: %u/%u)\n",
                item->id, item->state, item->completed, item->stages);
        list_del(&item->list);
        kfree(item);
    }
    mutex_unlock(&works_lock);

    destroy_workqueue(tsu_queue);
    pr_info("Workqueue destroyed\n");

    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Romanov");
MODULE_DESCRIPTION("Lab 4");
