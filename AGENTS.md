## `AGENTS.md`

### Purpose

This file provides operational guidance for agents executing inside a container. The container environment is pre-configured and **does not require network access** or dependency installation.

### ✅ Assumptions

* All build-time dependencies (e.g., compilers, CMake, Vulkan SDK, GLSLang, Doxygen) are installed.
* Git submodules (like `dsa/`) are already initialized and checked out.
* Internet access is **not permitted** during agent execution.

### 🧠 Agent Expectations

The agent must **not attempt to:**

* Fetch packages from the network.
* Re-clone or re-initialize Git submodules.
* Modify the container environment.
* Install software via package managers.

### 🛠 Usage Instructions

The agent may safely execute the following scripts from the project root:

#### Build the project

```sh
chmod +x build.sh
./build.sh
```

#### Run the Vulkan example with AddressSanitizer

```sh
chmod +x vk.sh
./vk.sh
```

#### Generate documentation (optional)

```sh
chmod +x doxy.sh
./doxy.sh
```

#### Run unit tests (optional)

```sh
ctest --rerun-failed --output-on-failure --test-dir build/
```

### 🔍 Expected Structure

The following directories and files **must exist** before the agent begins execution:

```
.
├── dsa/ # Git submodule initialized and populated
├── src/
├── include/
├── build.sh
├── vk.sh
├── doxy.sh
├── CMakeLists.txt
├── tests/
└── ...
```

### 🧪 Optional: Runtime Checks

Scripts such as `build.sh` may perform internal checks (e.g., verifying `dsa/CMakeLists.txt` exists) to ensure a sane build environment. These checks should not trigger network operations.
