
# func-riscv64-k230 单独启动测试的命令
cd /home/pc/qemu-camp-2026-k230/build && /home/pc/qemu-camp-2026-k230/build/pyvenv/bin/meson test --suite func-thorough func-riscv64-k230

or
```bash
/home/pc/qemu-camp-2026-k230/build/qemu-system-riscv64 \
    -M k230 \
    -kernel /home/pc/.cache/qemu/download/3a44970213fa68ad318d308518adfc0bf4bee72ed1b2926f9b468f82ef7d7829 \
    -dtb /home/pc/.cache/qemu/download/5050240b48ce0988c73eaefa73e4945a40abca503cf488d22a3adf6ef50bbe4c \
    -initrd /home/pc/.cache/qemu/download/4e1869a99a232ee60324f71f3a9e84a79b03ccabb5b73f8a727c5ff5be5c0914 \
    -append "printk.time=0 console=ttyS0,115200 earlycon=sbi" \
    -nographic
```


## buildroot qemu 启动命令
```bash
/home/pc/qemu-camp-2026-k230/build/qemu-system-riscv64 \
    -M k230 \
    -kernel /home/pc/.cache/qemu/download/113b5d2c01526566dd9ef37abb57788c9510f3c253b68276c784284d3e2b5cff \
    -dtb /home/pc/.cache/qemu/download/5050240b48ce0988c73eaefa73e4945a40abca503cf488d22a3adf6ef50bbe4c \
    -initrd /home/pc/.cache/qemu/download/42a664ab60342fbddf56419a422013c32ccebda598ff66a8d156797728eb096e \
    -append "printk.time=0 console=ttyS0,115200 earlycon=sbi cma=0" \
    -nographic
```
## 测试结果
