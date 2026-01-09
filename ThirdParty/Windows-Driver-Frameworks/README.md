## Windows Driver Frameworks (WDF) vendored headers

This folder vendors a small subset of headers from Microsoft’s **Windows Driver Frameworks** repository, to keep CI builds working on runners where the Windows Kits / WDK installation is incomplete.

- **Upstream repo**: `https://github.com/microsoft/Windows-Driver-Frameworks`
- **License**: MIT (see `LICENSE` in this folder)

### Included files

- `src/publicinc/wdf/umdf/2.15/wudfwdm.h`
  - Used to satisfy `#include <wudfwdm.h>` in the UMDF driver build when the header is not present in the runner’s Windows Kits install.

