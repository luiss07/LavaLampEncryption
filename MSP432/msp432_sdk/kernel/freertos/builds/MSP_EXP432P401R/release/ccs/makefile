
SIMPLELINK_MSP432_SDK_INSTALL_DIR ?= $(abspath ../../../../../..)

include $(SIMPLELINK_MSP432_SDK_INSTALL_DIR)/imports.mak

DRIVERLIB_DIR = $(SIMPLELINK_MSP432_SDK_INSTALL_DIR)
FREERTOSPORT_DIR = $(SIMPLELINK_MSP432_SDK_INSTALL_DIR)

VPATH = $(FREERTOS_INSTALL_DIR)/FreeRTOS/Source:$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/CCS/ARM_CM4F:$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/MemMang:$(SIMPLELINK_MSP432_SDK_INSTALL_DIR)/source:$(FREERTOSPORT_DIR)/kernel/freertos/dpl:$(FREERTOSPORT_DIR)/kernel/freertos/startup

CC = "$(CCS_ARMCOMPILER)/bin/armcl"
AR = "$(CCS_ARMCOMPILER)/bin/armar"

CFLAGS = -c -qq -pdsw225 -ms -g --endian=little -mv7M4 --float_support=fpv4spd16 --symdebug:dwarf -I.. "-I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/include" "-I$(FREERTOS_INSTALL_DIR)/FreeRTOS/Source/portable/CCS/ARM_CM4F" "-I$(SIMPLELINK_MSP432_SDK_INSTALL_DIR)/source/ti/posix/ccs" "-I$(FREERTOSPORT_DIR)/source" "-I$(DRIVERLIB_DIR)/source" "-I$(DRIVERLIB_DIR)/source/third_party/CMSIS/Include" "-I$(CCS_ARMCOMPILER)/include" -DDeviceFamily_MSP432P401x -D__MSP432P401R__

CFILES = \
    croutine.c \
    event_groups.c \
    list.c \
    queue.c \
    stream_buffer.c \
    tasks.c \
    timers.c \
    port.c \
    heap_4.c \
    ti/posix/freertos/clock.c \
    ti/posix/freertos/memory.c \
    ti/posix/freertos/mqueue.c \
    ti/posix/freertos/pthread_barrier.c \
    ti/posix/freertos/pthread_cond.c \
    ti/posix/freertos/pthread.c \
    ti/posix/freertos/pthread_mutex.c \
    ti/posix/freertos/pthread_rwlock.c \
    ti/posix/freertos/sched.c \
    ti/posix/freertos/semaphore.c \
    ti/posix/freertos/sleep.c \
    ti/posix/freertos/timer.c \
    ti/posix/freertos/PTLS.c \
    ti/posix/freertos/aeabi_portable.c \
    ClockP_freertos.c \
    DebugP_freertos.c \
    MutexP_freertos.c \
    SemaphoreP_freertos.c \
    SystemP_freertos.c \
    HwiPMSP432_freertos.c \
    PowerMSP432_freertos.c \
    startup_msp432_ccs.c

AFILES = \
    portasm.asm

OBJCFILES = $(subst /,_,$(CFILES:.c=.obj))
OBJAFILES = $(AFILES:.asm=.obj)

freertos.lib: $(OBJCFILES) $(OBJAFILES)
	@ echo ar $@ ...
	@ $(RM) $@ > $(DEVNULL) 2>&1
	@ $(AR) rq $@ $(OBJCFILES) $(OBJAFILES)

$(CFILES) $(AFILES):
	@ echo "Error: can't find $@, please ensure FREERTOS_INSTALL_DIR in $(SIMPLELINK_MSP432_SDK_INSTALL_DIR)/imports.mak is correct (it is '$(FREERTOS_INSTALL_DIR)')"
	@ exit 1

# Create a CC rule for each source file, where the object file
# name contains the source file path (replacing / with _).
#
define CC_RULE
$(subst /,_,$(1)).obj: $(1).c
	@ echo cc $$@ ...
	@ $$(CC) $$(CFLAGS) $$< -fe=$$@
endef
$(foreach file,$(subst .c,,$(CFILES)),$(eval $(call CC_RULE,$(file))))

%.obj:%.asm
	@ echo asm $@ ...
	@ $(CC) $(CFLAGS) $<

clean:
	@ echo cleaning ...
	@ $(RM) *.obj > $(DEVNULL) 2>&1
	@ $(RM) *.lib > $(DEVNULL) 2>&1
