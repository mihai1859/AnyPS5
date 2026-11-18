set -euo pipefail
mkdir -p build-queue-linux
common=(-std=c++20 -O2 -g -pthread -ffunction-sections -fdata-sections -Icore/libs)
for source in General GuestArena GuestWriteWatch; do
  g++ "${common[@]}" -c "core/libs/prx/libc/src/$source.cpp" -o "build-queue-linux/$source.o"
done
g++ "${common[@]}" -c core/libs/prx/libc/src/specifics/linux/GuestFaultCopy.cpp -o build-queue-linux/GuestFaultCopy.o
g++ "${common[@]}" -Dmain=QueueGuardMain -c core/libs/tests/GuestQueuePointerGuards.cpp -o build-queue-linux/guards.o
g++ "${common[@]}" -c verification/LinuxQueueVerification.cpp -o build-queue-linux/driver.o
for kind in baseline optimized; do
  source=core/libs/prx/libc/src/Queue.cpp
  if [ "$kind" = baseline ]; then source=verification/QueueBaseline.cpp; fi
  g++ "${common[@]}" -c "$source" -o "build-queue-linux/queue-$kind.o"
  g++ -pthread -Wl,--gc-sections -Wl,--wrap=GuestWritable_nid_no_patch build-queue-linux/General.o build-queue-linux/GuestArena.o build-queue-linux/GuestWriteWatch.o build-queue-linux/GuestFaultCopy.o build-queue-linux/guards.o build-queue-linux/driver.o "build-queue-linux/queue-$kind.o" -o "build-queue-linux/$kind"
  for mode in insque remque valid; do "build-queue-linux/$kind" "$mode"; done
  "build-queue-linux/$kind" volume "$kind"
done
build-queue-linux/optimized calls
for mode in insque remque valid calls; do APS5_GUEST_COPY_SYSCALL=1 build-queue-linux/optimized "$mode"; done
APS5_GUEST_COPY_SYSCALL=1 build-queue-linux/optimized volume optimized
