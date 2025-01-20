// SPDX-License-Identifier: GPL-2.0

#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>

static int snprint_greeting(char *s, size_t n, const char *who)
{
	return snprintf(s, n, "Hello %s!\n", who);
}

SYSCALL_DEFINE4(hello, char *, who, int, who_size, char *, buffer, int,
		buffer_size)
{
	char *who_kernel, *buffer_kernel;
	int greeting_size, err;

	if (who_size < 0 || who_size > PAGE_SIZE) {
		err = -EINVAL;
		goto err_who;
	}

	who_kernel = memdup_user_nul(who, who_size);
	if (IS_ERR(who_kernel)) {
		err = PTR_ERR(who_kernel);
		goto err_who;
	}

	greeting_size = snprint_greeting(NULL, 0, who_kernel);
	if (greeting_size > buffer_size) {
		err = -ETOOSMALL;
		goto err_greeting;
	}

	buffer_kernel = kzalloc(greeting_size, GFP_KERNEL);
	if (!buffer_kernel) {
		err = -ENOMEM;
		goto err_greeting;
	}

	snprint_greeting(buffer_kernel, greeting_size, who_kernel);
	err = copy_to_user(buffer, buffer_kernel, greeting_size);
	if (err) {
		err = -EFAULT;
		goto err_buffer;
	}

	kfree(buffer_kernel);
	kfree(who_kernel);
	return greeting_size;

err_buffer:
	kfree(buffer_kernel);
err_greeting:
	kfree(who_kernel);
err_who:
	return err;
}
