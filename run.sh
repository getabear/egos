qemu-system-riscv32 \
  -nographic \
  -machine sifive_u \
  -bios /home/bear/egos/egos-2000/tools/qemu/egos.bin \
  -m 16M \
  -smp 5 \
  -drive if=sd,format=raw,file=/home/bear/egos/egos-2000/tools/disk.img
  