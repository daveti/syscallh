/* sch
 * Syscall hijacking in 2019
 *
 * Feb 23, 2019
 * root@davejingtian.org
 * https://davejingtian.org
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>

static unsigned long *syscall_table;

static int __init sch_init(void)
{
	pr_info("sch: Entering: %s\n", __func__);

	syscall_table = (void *)kallsyms_lookup_name("sys_call_table");
	if (!syscall_table) {
		pr_err("sch: Couldn't look up sys_call_table\n");
		return -1;
	}
	pr_info("sch: syscall_table [%p]\n", syscall_table);

	return 0;
}

static void __exit sch_exit(void)
{
	pr_info("sch: exiting sch module\n");
}

module_init(sch_init);
module_exit(sch_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sch module");
MODULE_AUTHOR("daveti");

