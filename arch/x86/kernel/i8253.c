/*
 * Copyright (c) 2016-2020 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <lego/smp.h>
#include <lego/irq.h>
#include <lego/irqdesc.h>
#include <lego/jiffies.h>
#include <lego/spinlock.h>
#include <lego/clockevent.h>
#include <lego/clocksource.h>

#include <asm/io.h>
#include <asm/time.h>
#include <asm/timex.h>

/* i8253A PIT registers */
#define PIT_MODE	0x43
#define PIT_CH0		0x40
#define PIT_CH2		0x42

#define PIT_LATCH	((PIT_TICK_RATE + HZ/2) / HZ)

/*
 * Protects access to I/O ports
 *
 * 0040-0043 : timer0, i8253 / i8254
 * 0061-0061 : NMI Control Register which contains two speaker control bits.
 */
static DEFINE_SPINLOCK(i8253_lock);

/*
 * Since the PIT overflows every tick, its not very useful
 * to just read by itself. So use jiffies to emulate a free
 * running counter:
 */
static u64 i8253_read(struct clocksource *cs)
{
	static int old_count;
	static u32 old_jifs;
	unsigned long flags;
	int count;
	u32 jifs;

	spin_lock_irqsave(&i8253_lock, flags);
	/*
	 * Although our caller may have the read side of jiffies_lock,
	 * this is now a seqlock, and we are cheating in this routine
	 * by having side effects on state that we cannot undo if
	 * there is a collision on the seqlock and our caller has to
	 * retry.  (Namely, old_jifs and old_count.)  So we must treat
	 * jiffies as volatile despite the lock.  We read jiffies
	 * before latching the timer count to guarantee that although
	 * the jiffies value might be older than the count (that is,
	 * the counter may underflow between the last point where
	 * jiffies was incremented and the point where we latch the
	 * count), it cannot be newer.
	 */
	jifs = jiffies;
	outb_p(0x00, PIT_MODE);	/* latch the count ASAP */
	count = inb_p(PIT_CH0);	/* read the latched count */
	count |= inb_p(PIT_CH0) << 8;

	/* VIA686a test code... reset the latch if count > max + 1 */
	if (count > PIT_LATCH) {
		outb_p(0x34, PIT_MODE);
		outb_p(PIT_LATCH & 0xff, PIT_CH0);
		outb_p(PIT_LATCH >> 8, PIT_CH0);
		count = PIT_LATCH - 1;
	}

	/*
	 * It's possible for count to appear to go the wrong way for a
	 * couple of reasons:
	 *
	 *  1. The timer counter underflows, but we haven't handled the
	 *     resulting interrupt and incremented jiffies yet.
	 *  2. Hardware problem with the timer, not giving us continuous time,
	 *     the counter does small "jumps" upwards on some Pentium systems,
	 *     (see c't 95/10 page 335 for Neptun bug.)
	 *
	 * Previous attempts to handle these cases intelligently were
	 * buggy, so we just do the simple thing now.
	 */
	if (count > old_count && jifs == old_jifs)
		count = old_count;

	old_count = count;
	old_jifs = jifs;

	spin_unlock_irqrestore(&i8253_lock, flags);

	count = (PIT_LATCH - 1) - count;

	return (u64)(jifs * PIT_LATCH) + count;
}

static struct clocksource i8253_cs = {
	.name		= "pit",
	.rating		= 110,
	.read		= i8253_read,
	.mask		= CLOCKSOURCE_MASK(32),
};

int __init clocksource_i8253_init(void)
{
	return clocksource_register_hz(&i8253_cs, PIT_TICK_RATE);
}

static int pit_shutdown(struct clock_event_device *evt)
{
	if (!clockevent_state_oneshot(evt) && !clockevent_state_periodic(evt))
		return 0;

	spin_lock(&i8253_lock);
	outb_p(0x30, PIT_MODE);
	outb_p(0, PIT_CH0);
	outb_p(0, PIT_CH0);
	spin_unlock(&i8253_lock);

	return 0;
}

static int pit_set_oneshot(struct clock_event_device *evt)
{
	spin_lock(&i8253_lock);
	outb_p(0x38, PIT_MODE);
	spin_unlock(&i8253_lock);
	return 0;
}

static int pit_set_periodic(struct clock_event_device *evt)
{
	spin_lock(&i8253_lock);

	/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(0x34, PIT_MODE);

	/* LSB */
	outb_p(PIT_LATCH & 0xff, PIT_CH0);

	/* MSB */
	outb_p(PIT_LATCH >> 8, PIT_CH0);

	spin_unlock(&i8253_lock);
	return 0;
}

/*
 * Program the next event in oneshot mode
 *
 * Delta is given in PIT ticks
 */
static int pit_next_event(unsigned long delta, struct clock_event_device *evt)
{
	spin_lock(&i8253_lock);

	/* LSB */
	outb_p(delta & 0xff , PIT_CH0);

	/* MSB */
	outb_p(delta >> 8 , PIT_CH0);

	spin_unlock(&i8253_lock);
	return 0;
}


/*
 * On UP the PIT can serve all of the possible timer functions. On SMP systems
 * it can be solely used for the global tick.
 */
struct clock_event_device i8253_clockevent = {
	.name			= "pit",
	.event_handler		= tick_handle_periodic,
	.features		= CLOCK_EVT_FEAT_PERIODIC,
	.set_state_shutdown	= pit_shutdown,
	.set_state_periodic	= pit_set_periodic,
	.set_next_event		= pit_next_event,
};

/*
 * Initialize the conversion factor and the min/max deltas of the clock event
 * structure and register the clock event source with the framework.
 */
static void __init clockevent_i8253_init(bool oneshot)
{
	if (oneshot) {
		i8253_clockevent.features |= CLOCK_EVT_FEAT_ONESHOT;
		i8253_clockevent.set_state_oneshot = pit_set_oneshot;
	}

	/*
	 * Start pit with the boot cpu mask. x86 might make it global
	 * when it is used as broadcast device later.
	 */
	i8253_clockevent.cpumask = cpumask_of(smp_processor_id());

	clockevents_config_and_register(&i8253_clockevent, PIT_TICK_RATE,
					0xF, 0x7FFF);
}

void __init setup_pit_timer(void)
{
	clockevent_i8253_init(true);

	global_clock_event = &i8253_clockevent;
	pr_info("i8253 pit clockevent registered\n");
}
