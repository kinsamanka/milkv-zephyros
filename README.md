### Requirements
- Zephyr development environment: Follow [Getting Started Guide](https://docs.zephyrproject.org/3.6.0/develop/getting_started/index.html)
- Milk-V Duo: Install the latest [image](https://github.com/kinsamanka/milkv-zephyros/releases/download/v0.1.1-alpha/milkv-duo_sdcard.img.gz)

### Steps
- Clone repo
  ```
  git clone https://github.com/kinsamanka/milkv-zephyros
  ```
- Build firmware
  ```
  cd milkv-zephyros
  west init -l manifest-repo
  west update -o=--depth=1
  cmake -B build <blinky or openamp>
  make -C build
  ```
- Install firmware
  ```
  scp -O build/zephyr/zephyr.elf root@<milk-v ip addr>:/lib/firmware
  ```
- Run
  ```
  ssh root@<milk-v ip addr>
  echo stop > /sys/class/remoteproc/remoteproc0/state
  echo -n zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
  echo start > /sys/class/remoteproc/remoteproc0/state
  ```

### Note
- The OpenAMP sample needs an updated `remoteproc` and `mailbox` drivers. The sources can be found [here](https://github.com/kinsamanka/milkv-linux/tree/cvitek-v5.10.199)
