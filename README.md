## Licensing

- The source code of this project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.  
- This project includes third-party code:  
  - [MinOS](https://github.com/Dcraftbg/MinOS) by Dcraftbg, licensed under the **MIT License** (see `LICENSE.MINOS`).  
  - [Limine Bootloader](https://github.com/limine-bootloader/limine), licensed under the **BSD 3-Clause License** (see `kernel/vendor/limine/LICENSE`).  
- All third-party licenses are preserved in this repository as required.

### Visit our website:
https://lavaos.github.io/getlavaos/

![banner](banner.png)

# LavaOS 🔥

**LavaOS** is a free and open-source Unix-like operating system, forked from [MinOS](https://github.com/Dcraftbg/MinOS).  
It aims to be minimalist, hackable, and blazing fast — powered by a custom bootstrapping compiler (`nob.c`) and a raw, low-level design.

## 🚀 Features

- 🔥 Forked from MinOS with key improvements
- 🧠 Built using a custom bootstrapped compiler (`nob.c`)
- 🧩 Modular, simple, and readable codebase
- 🧑‍💻 Designed for OSDev hobbyists
- 💾 Light RAM/Swap-aware build system
- 🧪 Experimental but actively maintained

---

## 🔧 Getting Started

### 💾 Clone the source

```bash
git clone https://github.com/jrifuoue/LavaOS.git
cd LavaOS
```
### 🛠 Build the Compiler (nob)

```bash
gcc nob.c -o nob
```
### ⚙️ Build and Run the System

```bash
./nob bruh
```
## ⚠️ Heads Up

The first run will compile a full custom GCC toolchain inside your environment.
Make sure you have enough RAM + swap (recommendation: at least 4GB RAM + 8GB swap) to avoid out-of-memory errors.
Building may temporarily consume 10–60 GB of space depending on your setup.

## 🤝 Contributing

Contributions are more than welcome!

Feel free to open issues, PRs, or discussions. But please:

    Don’t intentionally obfuscate code

    Avoid adding heavy dependencies without solid reasoning

    Avoid large-scale rewrites unless they're clearly beneficial

    Let's keep LavaOS light, fun, and educational — together 💡
