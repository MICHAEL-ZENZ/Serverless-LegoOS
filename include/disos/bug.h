/*
 * Copyright (c) 2016 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DISOS_BUG_H_
#define _DISOS_BUG_H_

#include <disos/printk.h>
#include <disos/compiler.h>

/* Force a compilation error if a constant expression is not a power of 2 */
#define BUILD_BUG_ON_NOT_POWER_OF_2(n)			\
	BUILD_BUG_ON((n) == 0 || (((n) & ((n) - 1)) != 0))

/* Force a compilation error if condition is true, but also produce a
   result (of value 0 and type size_t), so the expression can be used
   e.g. in a structure initializer (or where-ever else comma expressions
   aren't permitted). */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define BUILD_BUG_ON_NULL(e) ((void *)sizeof(struct { int:-!!(e); }))

/*
 * BUILD_BUG_ON_INVALID() permits the compiler to check the validity of the
 * expression but avoids the generation of any code, even if that expression
 * has side-effects.
 */
#define BUILD_BUG_ON_INVALID(e) ((void)(sizeof((long)(e))))

/**
 * BUILD_BUG_ON_MSG - break compile if a condition is true & emit supplied
 *		      error message.
 * @condition: the condition which the compiler should know is false.
 *
 * See BUILD_BUG_ON for description.
 */
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)

/**
 * BUILD_BUG_ON - break compile if a condition is true.
 * @condition: the condition which the compiler should know is false.
 *
 * If you have some code which relies on certain constants being equal, or
 * some other compile-time-evaluated condition, you should use BUILD_BUG_ON to
 * detect if someone changes it.
 *
 * The implementation uses gcc's reluctance to create a negative array, but gcc
 * (as of 4.4) only emits that error for obvious cases (e.g. not arguments to
 * inline functions).  Luckily, in 4.3 they added the "error" function
 * attribute just for this type of case.  Thus, we use a negative sized array
 * (should always create an error on gcc versions older than 4.4) and then call
 * an undefined function with the error attribute (should always create an
 * error on gcc 4.3 and later).  If for some reason, neither creates a
 * compile-time error, we'll still have a link-time error, which is harder to
 * track down.
 */
#define BUILD_BUG_ON(condition) \
	BUILD_BUG_ON_MSG(condition, "BUILD_BUG_ON failed: " #condition)

/**
 * BUILD_BUG - break compile if used.
 *
 * If you have some code that you expect the compiler to eliminate at
 * build time, you should use BUILD_BUG to detect if it is
 * unexpectedly used.
 */
#define BUILD_BUG() BUILD_BUG_ON_MSG(1, "BUILD_BUG failed")

#define MAYBE_BUILD_BUG_ON(cond)			\
	do {						\
		if (__builtin_constant_p((cond)))       \
			BUILD_BUG_ON(cond);             \
		else                                    \
			BUG_ON(cond);                   \
	} while (0)


/*
 * Don't use BUG() or BUG_ON() unless there's really no way out; one
 * example might be detecting data structure corruption in the middle
 * of an operation that can't be backed out of.  If the (sub)system
 * can somehow continue operating, perhaps with reduced functionality,
 * it's probably not BUG-worthy.
 *
 * If you're tempted to BUG(), think again: is completely giving up
 * really the *only* solution? There are usually better options, where
 * users don't need to reboot ASAP and can mostly shut down cleanly.
 */

#define BUG()					\
do {						\
	printk("BUG: failure at %s:%d/%s()!\n",	\
		__FILE__, __LINE__, __func__);	\
	panic("BUG!");				\
} while(0)

#define BUG_ON(condition)			\
do {						\
	if (unlikely(condition))		\
		BUG();				\
} while (0)


/*
 * WARN(), WARN_ONCE(), WARN_ON(), WARN_ON_ONCE, are used to report
 * significant issues that need prompt attention if they should ever
 * appear at runtime.
 */

#define __WARN()						\
do {								\
	printk("WARNING: at %s:%d/%s()!\n",			\
		__FILE__, __LINE__, __func__);			\
} while(0)

#define __WARN_printk(format...)				\
do {								\
	__WARN();						\
	printk(format);						\
} while (0)

#define WARN(condition, format...) ({				\
	int __ret_warn_on = !!(condition);			\
	if (unlikely(__ret_warn_on))				\
		__WARN_printk(format);				\
	unlikely(__ret_warn_on);				\
})

#define WARN_ONCE(condition, format...) ({			\
	static bool __section(.data..unlikely) __warned;	\
	int __ret_warn_once = !!(condition);			\
								\
	if (unlikely(__ret_warn_once))				\
		if (WARN(!__warned, format))			\
			__warned = true;			\
	unlikely(__ret_warn_once);				\
})

#define WARN_ON(condition) ({					\
	int __ret_warn_on = !!(condition);			\
	if (unlikely(__ret_warn_on))				\
		__WARN();					\
	unlikely(__ret_warn_on);				\
})

#define WARN_ON_ONCE(condition)	({				\
	static bool __section(.data..unlikely) __warned;	\
	int __ret_warn_once = !!(condition);			\
								\
	if (unlikely(__ret_warn_once))				\
		if (WARN_ON(!__warned)) 			\
			__warned = true;			\
	unlikely(__ret_warn_once);				\
})

#endif /* _DISOS_BUG_H_ */
