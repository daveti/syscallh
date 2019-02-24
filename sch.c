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
#include <linux/fdtable.h>
//#include <linux/set_memory.h> /* newer kernel version */
#include <asm/cacheflush.h>	/* kernel 4.4 */

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
#endif

#ifdef AARCH64
#define SMP_UPDATE(x) \
	do { \
		preemtp_disable(); \
		x; \
		preempt_enable(); \
	} while (0)
#endif


static unsigned long *sys_call_table;
static typeof(sys_read) *orig_read;
static char *target = "sch-test";
static char *target_file = "README.md";


static void set_addr_rw(unsigned long _addr)
{
#ifdef X86_64
	unsigned int level;
	pte_t *pte = lookup_address(_addr, &level);

	if (pte->pte &~ _PAGE_RW)
		pte->pte |= _PAGE_RW;
#endif

#ifdef AARCH64
	set_memory_rw(_addr, 1);
	//return;
#endif
}

static void set_addr_ro(unsigned long _addr)
{
#ifdef X86_64
	unsigned int level;
	pte_t *pte = lookup_address(_addr, &level);

	pte->pte = pte->pte &~_PAGE_RW;
#endif

#ifdef AARCH64
	set_memory_ro(_addr, 1);
	//return;
#endif
}


static inline const char *basename(const char *hname)
{
	char *split;

	hname = strim((char *)hname);
	for (split = strstr(hname, "/"); split; split = strstr(hname, "/"))
		hname = split + 1;

	return hname;
}

static int is_target(int fd)
{
	char task_comm[TASK_COMM_LEN];
	struct files_struct *files;
	char *tmp;
	char *pathname;
	struct file *file;
	struct path *path;

	get_task_comm(task_comm, current);
	files = current->files;

	/* Check process name */
	if (strncmp(task_comm, target, TASK_COMM_LEN))
		return 0;

	/* Check file name */
	spin_lock(&files->file_lock);
	file = fcheck_files(files, fd);
	if (!file) {
		spin_unlock(&files->file_lock);
		return 0;
	}

	path = &file->f_path;
	path_get(path);
	spin_unlock(&files->file_lock);

	tmp = (char *)__get_free_page(GFP_KERNEL);
	if (!tmp) {
		path_put(path);
		return 0;
	}

	pathname = d_path(path, tmp, PAGE_SIZE);
	path_put(path);

	if (IS_ERR(pathname)) {
		free_page((unsigned long)tmp);
		return 0;
	}

	pr_info("sch: is_target [%s]\n", pathname);

	if (strncmp(basename(pathname), target_file, strlen(target_file))) {
		free_page((unsigned long)tmp);
		return 0;
	}

	/* Here we are */
	free_page((unsigned long)tmp);
	return 1;

}

asmlinkage long my_read(int fd, char __user *buf, size_t count)
{
	unsigned char kbuf[32];

	/* Only respond to the test program with the target file */
	if (is_target(fd)) {
		pr_info("sch: my_read - intercept target\n");
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

#ifdef X86_64
	pr_info("sch: arch x86_64\n");
#endif

#ifdef AARCH64
	pr_info("sch: arch aarch64\n");
#endif
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
