#include <linux/module.h>   
#include <linux/kernel.h>   
#include <linux/proc_fs.h>  
#include <linux/seq_file.h> 
#include <linux/slab.h>     
#include <linux/timekeeping.h>
#include <linux/jiffies.h>
#include <linux/math64.h> 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tomsk State University Student");
MODULE_DESCRIPTION("Module with /proc interface for the Halley's Comet task.");

#define PROCFS_NAME "tsulab" 
static struct proc_dir_entry *proc_file; 

#define HALLEY_PERIOD_DAYS (76LL * 365LL + 19LL)
#define HALLEY_PERIOD_SECONDS (HALLEY_PERIOD_DAYS * 86400LL)

#define LAST_PERIHELION_TIMESTAMP 508248000LL 

static int tsu_show(struct seq_file *m, void *v) {
    u64 now_sec = ktime_get_real_seconds();
    u64 seconds_passed = now_sec - LAST_PERIHELION_TIMESTAMP;
    
    u64 current_cycle_seconds = seconds_passed;
    u64 total_cycles = div_u64_rem(current_cycle_seconds, HALLEY_PERIOD_SECONDS, &current_cycle_seconds); 
    
    u64 percentage_scaled_10000 = (current_cycle_seconds * 10000LL);
    do_div(percentage_scaled_10000, HALLEY_PERIOD_SECONDS);

    u64 whole_part = percentage_scaled_10000 / 100;
    u64 fractional_part = percentage_scaled_10000 % 100;

    u64 years_passed = seconds_passed;
    do_div(years_passed, (365LL * 86400LL));

    seq_printf(m, "--- Tomsk State University Lab ---\n");
    seq_printf(m, "Прошло %llu секунд с последнего перигелия.\n", seconds_passed);
    seq_printf(m, "Текущий цикл (UTC): %llu (дней) / %llu (дней)\n", 
               current_cycle_seconds / 86400LL, HALLEY_PERIOD_DAYS);
    seq_printf(m, "Комета Галлея пролетела %llu.%02llu%% своего круга между перегелиями.\n", 
               whole_part, fractional_part);
    seq_printf(m, "--- End of TSU Lab Data ---\n");
    
    return 0;
}

static int tsu_open(struct inode *inode, struct file *file) {
    return single_open(file, tsu_show, NULL);
}

static const struct proc_ops tsu_proc_ops = {
    .proc_open    = tsu_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init tsu_init(void)
{
    printk(KERN_INFO "Welcome to the Tomsk State University\n");
    proc_file = proc_create(PROCFS_NAME, 0644, NULL, &tsu_proc_ops);
    
    if (proc_file == NULL) {
        printk(KERN_ERR "TSU Module: Ошибка: Не удалось создать файл /proc/%s\n", PROCFS_NAME);
        return -ENOMEM; 
    }
    
    printk(KERN_INFO "TSU Module: /proc/%s успешно создан и готов к чтению\n", PROCFS_NAME);

    return 0;
}

static void __exit tsu_exit(void)
{
    remove_proc_entry(PROCFS_NAME, NULL);
    printk(KERN_INFO "TSU Module: /proc/%s удален\n", PROCFS_NAME);
    printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(tsu_init);
module_exit(tsu_exit);
