# nymea-gpio

This repository contains `libnymea-gpio`, a reusable Qt/C++ helper library that allows nymea and other applications to configure and interact with general purpose I/O pins on supported platforms, and `nymea-gpio-tool`, a small CLI utility that exposes the same functionality for scripting and diagnostics.

Both components are developed together to keep their pin-handling logic and configuration formats in sync, so the command line tool doubles as a reference implementation for the library.

## License

- `libnymea-gpio` is licensed under the terms of the GNU Lesser General Public License v3.0 or (at your option) any later version. See `LICENSE.LGPL3` for the full text.
- `nymea-gpio-tool` is licensed under the terms of the GNU General Public License v3.0 or (at your option) any later version. See `LICENSE.GPL3` for the full text.
