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


static int __init sch_init(void)
{
	//int ret;

	pr_info("sch: Entering: %s\n", __func__);

	return 0;
}

static void __exit sch_exit(void)
{
	pr_info("exiting sch module\n");
}

module_init(sch_init);
module_exit(sch_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sch module");
MODULE_AUTHOR("daveti");

