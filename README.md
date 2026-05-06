<div align="center">
  <img src="banner.png" alt="LavaOS Banner" width="800">
  
  <h1>🌋 LavaOS</h1>
  
  <p>
    <strong>A free and open-source Unix-like operating system, forked from <a href="https://github.com/Dcraftbg/MinOS">MinOS</a>.</strong><br>
    Minimalist. Hackable. Blazing fast — powered by a custom bootstrapping compiler (<code>nob.c</code>) and raw, low-level design.
  </p>

  <p>
    <a href="https://github.com/LavaOS/LavaOS">
      <img src="https://img.shields.io/badge/Architecture-x86__64%20(64--bit%20only)-red?style=for-the-badge&logo=archlinux" alt="Architecture">
    </a>
    <a href="https://github.com/LavaOS/LavaOS/blob/main/LICENSE">
      <img src="https://img.shields.io/badge/License-GPLv3-blue?style=for-the-badge&logo=gnu" alt="License">
    </a>
    <a href="https://github.com/LavaOS/LavaOS">
      <img src="https://img.shields.io/badge/Language-C%20%7C%20Assembly-orange?style=for-the-badge&logo=c" alt="Language">
    </a>
    <a href="https://lavaos.github.io/getlavaos/">
      <img src="https://img.shields.io/badge/Website-Visit%20Us-brightgreen?style=for-the-badge&logo=firefox" alt="Website">
    </a>
  </p>
  
  <p>
    <a href="https://github.com/LavaOS/LavaOS/stargazers">
      <img src="https://img.shields.io/github/stars/LavaOS/LavaOS?style=social" alt="Stars">
    </a>
    <a href="https://github.com/LavaOS/LavaOS/network/members">
      <img src="https://img.shields.io/github/forks/LavaOS/LavaOS?style=social" alt="Forks">
    </a>
    <a href="https://github.com/LavaOS/LavaOS/issues">
      <img src="https://img.shields.io/github/issues/LavaOS/LavaOS?style=social" alt="Issues">
    </a>
  </p>
</div>

---

## 📸 Screenshots

<table align="center">
  <tr>
    <td align="center"><img src="screenshots/cat.png" width="300"><br><sub>🐱 cat command</sub></td>
    <td align="center"><img src="screenshots/desktop.png" width="300"><br><sub>🖥️ Desktop</sub></td>
  </tr>
  <tr>
    <td align="center"><img src="screenshots/dim.png" width="300"><br><sub>📝 DIM Editor</sub></td>
    <td align="center"><img src="screenshots/doomgeneric.png" width="300"><br><sub>🎮 DOOM</sub></td>
  </tr>
  <tr>
    <td align="center" colspan="2"><img src="screenshots/login.png" width="300"><br><sub>🔐 Login Screen</sub></td>
  </tr>
</table>

---

## 🚀 Features

<table align="center">
  <tr>
    <td align="center" width="33%">
      <h3>🔥 Forked & Enhanced</h3>
      <p>Built from <a href="https://github.com/Dcraftbg/MinOS">MinOS</a> with significant improvements and optimizations</p>
    </td>
    <td align="center" width="33%">
      <h3>🧠 Custom Compiler</h3>
      <p>Bootstrapped with <code>nob.c</code> — a custom C compiler tailored for LavaOS</p>
    </td>
    <td align="center" width="33%">
      <h3>🧩 Modular Design</h3>
      <p>Clean, readable, and easy to hack — perfect for OSDev enthusiasts</p>
    </td>
  </tr>
  <tr>
    <td align="center">
      <h3>💾 RAM-Friendly</h3>
      <p>Builds with as little as 4GB RAM + 8GB swap — swap-aware build system</p>
    </td>
    <td align="center">
      <h3>🧪 Experimental</h3>
      <p>Actively maintained with bleeding-edge features and constant improvements</p>
    </td>
    <td align="center">
      <h3>🎯 Low-Level</h3>
      <p>Raw, bare-metal design, no bloatware</p>
    </td>
  </tr>
</table>

---

## 🔧 Getting Started

<div align="center">
  <table>
    <tr>
      <td align="center" width="25%">
        <h3>📦 1. Clone</h3>
        <code>git clone https://github.com/LavaOS/LavaOS.git</code><br>
        <code>cd LavaOS</code>
      </td>
      <td align="center" width="25%">
        <h3>🔨 2. Build Compiler</h3>
        <code>gcc nob.c -o nob</code>
      </td>
      <td align="center" width="25%">
        <h3>🔨 3. Build OS</h3>
        <code>./nob build</code>
      </td>
      <td align="center" width="25%">
        <h3>⚡ 4. Run</h3>
        <code>./nob run</code>
      </td>
    </tr>
  </table>
</div>

> <details>
> <summary><b>⚠️ Heads Up — First Build Warning</b></summary>
> <br>
> The first run will compile a full custom GCC toolchain inside your environment.<br>
> Make sure you have enough <b>RAM + swap</b> (recommendation: at least <b>4GiB RAM + 8GiB swap</b>) to avoid out-of-memory errors.<br>
> Building may temporarily consume <b>1–10 GiB</b> of space depending on your setup.<br><br>
> <b>Recommended distro for build:</b> Arch Linux
> </details>

---

## 📜 Licensing

<div align="center">
  <table>
    <tr>
      <td align="center" width="50%">
        <h3>🔑 Main License</h3>
        <a href="https://www.gnu.org/licenses/gpl-3.0.html">
          <img src="https://img.shields.io/badge/GNU-GPLv3-red?style=for-the-badge&logo=gnu">
        </a>
        <p>The source code of this project is licensed under the <strong>GNU General Public License v3.0</strong></p>
      </td>
      <td align="center" width="50%">
        <h3>📦 Third-Party Code</h3>
        <p>All third-party licenses are preserved in this repository as required</p>
      </td>
    </tr>
  </table>
</div>

<details>
<summary><b>Click to expand third-party licenses</b></summary>
<br>
<table>
  <tr><th>Project</th><th>Author</th><th>License</th><th>Location</th></tr>
  <tr>
    <td><a href="https://github.com/Dcraftbg/MinOS">MinOS</a></td>
    <td>Dcraftbg</td>
    <td>MIT</td>
    <td><code>licenses/LICENSE.MINOS</code></td>
  </tr>
  <tr>
    <td><a href="https://github.com/limine-bootloader/limine">Limine Bootloader</a></td>
    <td>Limine Team</td>
    <td>BSD 3-Clause</td>
    <td><code>kernel/vendor/limine/LICENSE</code></td>
  </tr>
  <tr>
    <td><a href="https://github.com/Dcraftbg/doomgeneric">Doomgeneric</a></td>
    <td>dcraftbg</td>
    <td>GPLv2</td>
    <td><code>user/doomgeneric/LICENSE</code></td>
  </tr>
  <tr>
    <td><a href="https://github.com/Dcraftbg/dim">DIM Editor</a></td>
    <td>dcraftbg</td>
    <td>MIT</td>
    <td><code>licenses/LICENSE.DIM</code></td>
  </tr>
  <tr>
    <td><a href="https://github.com/amirh1385/Sinux">Sinux Kernel</a></td>
    <td>amirh1385 (fork)</td>
    <td>MIT</td>
    <td><code>licenses/LICENSE.SINUX</code></td>
  </tr>
  <tr>
    <td><a href="https://github.com/torvalds/linux/tree/v3.0">Linux Kernel 3.0</a></td>
    <td>Linus Torvalds</td>
    <td>GPLv2</td>
    <td><code>licenses/LICENSE.LINUX3</code></td>
  </tr>
</table>
</details>

---

## ⭐ Star History

<div align="center">
  <a href="https://star-history.com/#LavaOS/LavaOS&Date">
    <img src="https://api.star-history.com/svg?repos=LavaOS/LavaOS&type=Date" alt="Star History Chart" width="600">
  </a>
</div>

---

## 🤝 Contributing

<div align="center">
  <table>
    <tr>
      <td align="center">✅ Open Issues & PRs</td>
      <td align="center">✅ Join Discussions</td>
    </tr>
    <tr>
      <td align="center">❌ Don't obfuscate code</td>
      <td align="center">❌ Avoid heavy dependencies</td>
    </tr>
    <tr>
      <td align="center" colspan="2">❌ No large-scale rewrites unless clearly beneficial</td>
    </tr>
  </table>
  
  <br>
  
  <h3>Let's keep LavaOS light, fun, and educational — together 💡</h3>
</div>

---

<div align="center">
  <p>
    <a href="https://lavaos.github.io/getlavaos/">
      <img src="https://img.shields.io/badge/🌐%20Website-lavaos.github.io/getlavaos-blue?style=for-the-badge">
    </a>
  </p>
  
  <p>Made in Iran by the LavaOS Team</p>
</div>
