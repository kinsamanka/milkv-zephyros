### Requirements
- Zephyr development environment: Follow [Getting Started Guide](https://docs.zephyrproject.org/3.6.0/develop/getting_started/index.html)
- Milk-V Duo: Install the latest [Arduino image](https://github.com/milkv-duo/duo-buildroot-sdk/releases)
  - Make sure the led blinking task is not enabled

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
  cmake -B build blinky
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

### Results
- UART4 Tx (pin 4) should be active
- Blue led should be blinking
