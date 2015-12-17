#include <linux/kernel.h>
#include <linux/module.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_math.h>

#include "pci20k.c"
#include "zib1155.c"

#define PERIOD 25
#define TIMER 1

#define LARGE 13.2
#define SMALL 4.4

#define LARGE_COUNT 1400
#define SMALL_COUNT 400

#define BALL 10.0
#define DISK 5.0
#define TICKS 2048.0
#define GRAVITY 9.81
#define DEGREES 360.0

#define MILLIMETERS_PER_METER 1000
#define MILLISECONDS_PER_SECOND 1000.0
#define NANOSECONDS_PER_MILLISECOND 1000000.0
#define NANOSECONDS_PER_SECOND 1000000000.0

#define MAX_VOLTS 10.0
#define MIN_DISTANCE 0.1
#define MAX_DISTANCE 0.5
#define DISTANCE_BIAS 0.019

static int mod(int a, int b) {
	return ((a % b) + b) % b;
}

static void release(void) {
	digital_ausgabe(0, 0xff);
	rt_sleep(nano2count(25 * NANOSECONDS_PER_MILLISECOND));
	digital_ausgabe(0, 0x00);
}

static float measure_distance(void) {
	float volts = analog_eingabe(7);
	float distance = volts / MAX_VOLTS * (MAX_DISTANCE - MIN_DISTANCE) + MIN_DISTANCE - DISTANCE_BIAS;

	return distance;
}

static int measure_position(void) {
	int ticks = ZIBGetCounter(2);
	return ticks;
}

static float average(float* values, int length) {
	float value = 0;
	
	int i;
	for(i = 0; i < length; i++) {
		value += values[i];
	}
	value /= (float) length;

	return value;
}

// Calculate fall time in seconds, given fall height in meters
static float fall_time(float height) {
	return sqrt(2 * height) / sqrt(GRAVITY);
}

// determine whether the ball can make it through a hole when falling from the given height in meters and spinning at the given ticks per second
static int possible(float holeSize, float height, int tps) {
	int max = holeSize / DEGREES / (BALL + DISK) * MILLIMETERS_PER_METER * GRAVITY * fall_time(height) * TICKS;
	return tps < max;
}

#define MEASUREMENTS 10

static void debug(long t) {
	float counter[MEASUREMENTS] = { 0.0 };
	int index = 0;

	int previous = measure_position();

	while(1) {
		int count = measure_position();
		counter[index] = mod(count - previous, TICKS);
		previous = count;
		
		index = mod(index + 1, MEASUREMENTS);

		float value = average(counter, MEASUREMENTS);
		value = value / ((float) PERIOD / MILLISECONDS_PER_SECOND);

		float distance = measure_distance();

		rt_printk("%d millimeters\n", (int) (distance * MILLIMETERS_PER_METER));
		rt_printk("%d tps\n", (int) (value));
		rt_printk("%d ticks\n", count);
		rt_printk("possible: small %d, large %d\n", possible(SMALL, distance, (int) value), possible(LARGE, distance, (int) value));

		//release();

		rt_sleep(nano2count(PERIOD * NANOSECONDS_PER_MILLISECOND));
	}
}

static void handler(long t) {
	float height = measure_distance();

	rt_printk("\n");
	rt_printk("Height is %d mm\n", (int) (height * MILLIMETERS_PER_METER));

	float counter[MEASUREMENTS] = { 0.0 };
	int previous = measure_position();

	int i;
	for(i = 0; i < MEASUREMENTS; i++) {
		rt_sleep(nano2count(PERIOD * NANOSECONDS_PER_MILLISECOND));

		int count = measure_position();
		counter[i] = mod(count - previous, TICKS);
		previous = count;
	}

	float tps = average(counter, MEASUREMENTS) / ((float) PERIOD / MILLISECONDS_PER_SECOND);

	rt_printk("Turn speed is %d tps\n", (int) tps);

	if(possible(LARGE, height, (int) tps)) {
		int count = measure_position();

		int drop_count = mod(LARGE_COUNT - tps * fall_time(height), TICKS);

		rt_printk("Current position is %d ticks, drop position is %d ticks\n", count, drop_count);
		
		int wait_ticks = mod(drop_count - count, TICKS);

		long long wait_time = (long long) ((float) wait_ticks / tps * NANOSECONDS_PER_SECOND);

		rt_printk("Waiting %d ticks (%lld nanoseconds)\n", wait_ticks, wait_time);

		rt_sleep(nano2count(wait_time));

		count = measure_position();
		
		rt_printk("Dropping at %d ticks (off by %d)\n", count, count - drop_count);

		release();
	}
	else {
		rt_printk("Not possible\n");
	}
}

static RT_TASK task;

static __init int init(void) {
	rt_mount();

	rt_linux_use_fpu(1);

	rt_set_oneshot_mode();
	start_rt_timer(0);

	rt_task_init(&task, handler, 0, 4096, 4, 1, 0);

	init_pci();

	rt_task_resume(&task);
	
	return 0;
}

static __exit void deinit(void) {
	stop_rt_timer();
	rt_task_delete(&task);
	rt_umount();
}

module_init(init);
module_exit(deinit);
