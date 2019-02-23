/* sch
 * Syscall hijacking in 2019
 * Feb 23, 2019
 * root@davejingtian.org
 * https://davejingtian.org
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h>

/* SMP handling */
#ifdef X86_64
#define SMP_UPDATE(x) \
	do { \
		unsigned long __cr0; \
		preempt_disable(); \
		__cr0 = read_cr0() & (~X86_CR0_WP); \
		write_cr0(__cr0); \
		x; \
		__cr0 = read_cr0() | X86_CR0_WP; \
		write_cr0(__cr0); \
		preempt_enable(); \
	} while (0)
#elif  AARCH64
#define SMP_UPDATE(x) \
	do { \
		preemtp_disable(); \
		x; \
		preempt_enable(); \
	} while (0)
#else
#define SMP_UPDATE(x) \
	do { \
		x; \
	} while (0)
#endif


static unsigned long *sys_call_table;
static typeof(sys_read) *orig_read;
static char *target = "sch-test";


static void set_addr_rw(unsigned long _addr)
{
	unsigned int level;
	pte_t *pte = lookup_address(_addr, &level);

	if (pte->pte &~ _PAGE_RW)
		pte->pte |= _PAGE_RW;
}

static void set_addr_ro(unsigned long _addr)
{
	unsigned int level;
	pte_t *pte = lookup_address(_addr, &level);

	pte->pte = pte->pte &~_PAGE_RW;
}



asmlinkage long my_read(int fd, char __user *buf, size_t count)
{
	unsigned char kbuf[32];
	char task_comm[TASK_COMM_LEN];

 	get_task_comm(task_comm, current);

	/* Only respond to the test program */
	if (!strncmp(task_comm, target, TASK_COMM_LEN)) {
		memset(kbuf, '7', sizeof(kbuf));
		copy_to_user(buf, kbuf, sizeof(kbuf));
		return sizeof(kbuf);
	}

	/* Call the original syscall */
	return orig_read(fd, buf, count);
}


static void syscall_hijack(void)
{
	orig_read = (typeof(sys_read) *)sys_call_table[__NR_read];
	pr_info("sch: orig sys_read [%p]\n", orig_read);

	set_addr_rw((unsigned long)sys_call_table);
	SMP_UPDATE({sys_call_table[__NR_read] = (void *)&my_read;});
}

static void syscall_recover(void)
{
	SMP_UPDATE({sys_call_table[__NR_read] = (void *)orig_read;});
	set_addr_ro((unsigned long)sys_call_table);
}


static int __init sch_init(void)
{
	pr_info("sch: Entering: %s\n", __func__);

	sys_call_table = (void *)kallsyms_lookup_name("sys_call_table");
	if (!sys_call_table) {
		pr_err("sch: Couldn't look up sys_call_table\n");
		return -1;
	}
	pr_info("sch: sys_call_table [%p]\n", sys_call_table);

	syscall_hijack();

	return 0;
}

static void __exit sch_exit(void)
{
	pr_info("sch: exiting sch module\n");
	syscall_recover();
}

module_init(sch_init);
module_exit(sch_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sch module");
MODULE_AUTHOR("daveti");
