# `AGENTS.md`

## 🎯 Purpose

This file provides operational guidance for agents executing inside a containerized environment. The container is **pre-configured**, and **network access is disabled** during execution.

## ✅ Assumptions

* All build-time dependencies are installed:

  * Compilers (`gcc`, `clang`)
  * `cmake`
  * Vulkan SDK
  * GLSLang tools (`glslangValidator`)
  * `doxygen`
* Git submodules (like `dsa/`) are **already initialized** and do **not** require cloning.
* Internet access is **not available** at runtime.

## 🧠 Agent Behavior Requirements

The agent **must not attempt to**:

* Fetch packages or files from the internet.
* Clone, update, or initialize Git submodules.
* Modify the container environment or install additional tools.
* Assume persistent writable storage beyond the project directory.

## 🛠️ Usage Instructions

The agent may safely execute the following scripts from the project root:

### 🔧 Build the project

```sh
chmod +x build.sh
./build.sh
```

### 🧪 Run the Vulkan example (AddressSanitizer enabled)

```sh
chmod +x vk.sh
./vk.sh
```

### 📚 Generate documentation (optional)

```sh
chmod +x doxy.sh
./doxy.sh
```

### ✔️ Run unit tests (optional)

```sh
ctest --rerun-failed --output-on-failure --test-dir build/
```

## 📁 Required Directory Layout

The following files and directories **must be present** before agent execution begins:

```
.
├── dsa/              # Pre-initialized submodule or extracted archive
├── dsa.tar.gz        # Backup archive of dsa/, for offline fallback
├── src/
├── include/
├── build.sh
├── init.sh           # Unpacks dsa.tar.gz if needed
├── vk.sh
├── doxy.sh
├── CMakeLists.txt
├── tests/
└── ...
```

## 📦 Offline Submodule Initialization

This project uses the `dsa/` library as a Git submodule. For offline environments, a fallback archive `dsa.tar.gz` is included.

### 🧰 Step 1: Extract the Submodule

```sh
chmod +x init.sh
./init.sh
```

This script checks if `dsa/` exists and unpacks `dsa.tar.gz` if necessary.

### 🧪 Step 2: Build the Project

```sh
./build.sh
```

No further setup is required after `init.sh` completes successfully.

## 🔒 Security & Isolation

* Do **not** run `git submodule update` — this environment is offline.
* Do **not** delete or move `dsa.tar.gz` unless `dsa/` is fully extracted.
* Avoid adding new runtime dependencies or reaching out to external services.

## 🧠 Final Note

This setup ensures agents operate deterministically, offline, and with full access to the required source tree. If `dsa/` is missing or corrupted, fallback via `init.sh` ensures resilience.
