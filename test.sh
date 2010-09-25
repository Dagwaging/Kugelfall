rmmod kugelfall.ko

insmod /usr/realtime/modules/rtai_hal.ko
insmod /usr/realtime/modules/rtai_ksched.ko
insmod /usr/realtime/modules/rtai_math.ko

insmod kugelfall.ko
