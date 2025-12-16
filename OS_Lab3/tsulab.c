#include <linux/module.h>   
#include <linux/kernel.h>   
#include <linux/proc_fs.h>  
#include <linux/seq_file.h> 
#include <linux/slab.h>     

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tomsk State University Student");
MODULE_DESCRIPTION("Module with /proc interface for the Halley's Comet task.");

#define PROCFS_NAME "tsulab" 
static struct proc_dir_entry *proc_file; 

#define HALLEY_PERIOD_YEARS 76
#define LAST_PERIHELION_YEAR 1986
#define CURRENT_YEAR 2025 

static int tsu_show(struct seq_file *m, void *v) {
    int years_passed = CURRENT_YEAR - LAST_PERIHELION_YEAR;
    int percentage_scaled = (years_passed * 1000) / HALLEY_PERIOD_YEARS; 
    
    int whole_part = percentage_scaled / 10;
    int fractional_part = percentage_scaled % 10;

    seq_printf(m, "--- Tomsk State University Lab ---\n");
    seq_printf(m, "Прошло %d лет с последнего перегелия (1986).\n", years_passed);
    seq_printf(m, "Период обращения кометы Галлея: %d лет.\n", HALLEY_PERIOD_YEARS);
    seq_printf(m, "Комета Галлея пролетела %d.%d%% своего круга между перегелиями.\n", whole_part, fractional_part);
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