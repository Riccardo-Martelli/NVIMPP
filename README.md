<h1 align="center">Nvimpp and Nvimi</h1>

<h3 align="center">Frecency tracking for Neovim, in the spirit of <a href="https://github.com/ajeetdsouza/zoxide">zoxide</a> and <a href="https://github.com/atuinsh/atuin">atuin</a>.</h3>

<p align="center">
  <a href="https://github.com/Riccardo-Martelli/nvimpp/tags">
    <img src="https://img.shields.io/github/v/tag/Riccardo-Martelli/nvimpp?label=version&color=red">
  </a>
  <a href="./LICENSE">
    <img src="https://img.shields.io/github/license/Riccardo-Martelli/nvimpp?color=blue">
  </a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue">
  <img src="https://img.shields.io/badge/contributions-welcome-orange">
  <a href="https://github.com/Riccardo-Martelli/nvimpp/stargazers">
    <img src="https://img.shields.io/github/stars/Riccardo-Martelli/nvimpp">
  </a>
  <a href="https://github.com/Riccardo-Martelli/nvimpp/network/members">
    <img src="https://img.shields.io/github/forks/Riccardo-Martelli/nvimpp">
  </a>
</p>

<img src="media/divider.svg" width="100%" height="4">

* **nvimpp** is a thin wrapper around `nvim` (or `vim`, see [Choosing the editor](#choosing-the-editor)). It records every launch (date, resolved file path, arguments) in a history file, then hands over to the editor with the arguments untouched. With `--find` it jumps to the best matching file from the history instead of opening the literal name.
* **nvimi** reads that history. Its default mode is an inline picker that ranks files by frecency (frequency weighted by recency) and opens the selected one. It also has a `stats` and a `history` mode.

<img src="media/divider.svg" width="100%" height="4">

## Contents

* [Requirements](#requirements)
* [Compatibility](#compatibility)
* [Build and install](#build-and-install)
* [Setup](#setup)
* [Usage](#usage)
  * [nvimpp](#nvimpp)
  * [nvimi](#nvimi)
* [Configuration](#configuration)
  * [Choosing the editor](#choosing-the-editor)
  * [Format rules](#format-rules)
* [Scoring](#scoring)
* [History file](#history-file)
* [Exit status](#exit-status)
* [Man pages](#man-pages)
* [Limitations and future work](#limitations-and-future-work)
* [Maintainer](#maintainer)
* [License](#license)

<img src="media/divider.svg" width="100%" height="4">

## Requirements

* Linux, a C++17 compiler (`g++`), `make`
* Neovim (`nvim`) in `PATH`, or Vim (`vim`) if you select it in `editors.config`
* bash or zsh, for the optional Alt-Enter binding

<img src="media/divider.svg" width="100%" height="4">

## Compatibility

| System | Status |
|---|---|
| Linux | supported, developed and tested on Fedora |
| macOS, FreeBSD and other BSDs | not tested; the code only uses POSIX (Portable Operating System Interface) features such as `termios`, `/dev/tty` and `execvp`, so it may build, but nothing is guaranteed |
| Windows | not supported natively; inside WSL (Windows Subsystem for Linux) it behaves like Linux, but this is not tested |

Reports from other systems are welcome: open an issue with your OS and compiler version.

<img src="media/divider.svg" width="100%" height="4">

## Build and install

```bash
git clone https://github.com/Riccardo-Martelli/nvimpp.git
cd nvimpp
make
```

A single `make` builds both programs and installs:

* `nvimpp` and `nvimi` into `~/.local/bin` (make sure that folder is in your `PATH`)
* the man pages into `~/.local/share/man/man1`
* an empty history file into `~/.local/share/nvimpp`
* the default configs into `~/.config/nvimpp/nvimpp.config` and `~/.config/nvimpp/editors.config`, each only if you don't have it yet

Running `make` again after an update rebuilds and reinstalls the programs and keeps your config and history. If you installed a version without `editors.config`, the same `make` adds it next to your existing config.

To install the programs and man pages somewhere else:

```bash
make PREFIX=/some/prefix
```

To remove the programs and man pages (your config and history stay):

```bash
make uninstall
```

<img src="media/divider.svg" width="100%" height="4">

## Setup

Add to `~/.bashrc` (or `~/.zshrc`):

```bash
alias nvim=nvimpp
eval "$(nvimpp --init bash)" # or: eval "$(nvimpp --init zsh)"
```

The alias makes every `nvim` you type go through `nvimpp`. Programs that call Neovim themselves (`git commit`, `sudoedit`, scripts) still use the real `nvim`, so their files are not recorded. Typing `\nvim` or `command nvim` also skips the wrapper.

The `eval` line installs the Alt-Enter key binding described below.

<img src="media/divider.svg" width="100%" height="4">

## Usage

### nvimpp

```bash
nvimpp file.cpp # open file.cpp and record it
nvimpp a.cpp b.cpp # each file gets its own history line
nvimpp # no file: records the current folder as a dashboard entry
nvimpp -R notes.md # Neovim flags are passed through unchanged
nvimpp query --find # open the best match for "query" from the history
nvimpp --vim file.cpp # use Vim for this launch, whatever editors.config says
```

**Enter vs Alt-Enter.** With the binding installed, a line starting with `nvim ` or `nvimpp ` behaves like this:

* **Enter** opens exactly what you typed. `nvim main` opens or creates the file `main`.
* **Alt-Enter** appends `--find`. `nvim main` then opens the most used and most recent file in the history whose name contains `main`, for example `~/project/src/main.cpp`.

**How `--find` matches** (tip: Alt-Enter is the cooler way to use it). All non-flag words form one query.

* Matching is case-insensitive.
* The words must appear in the file path in the order typed.
* The last word must appear in the file name itself, not only in a folder name.
* Only files that still exist on disk are candidates.
* The candidate with the highest frecency score wins. If nothing matches, `nvimpp` prints `no match for the query` and Neovim does not start.

```bash
nvimpp conway cgof --find # matches .../conwaygof/cgof.cpp
nvimpp cgof conway --find # no match: wrong order
```

### nvimi

```bash
nvimi # inline picker, ranked by frecency
nvimi stats # files ranked by number of launches
nvimi history # every launch with date and time since
nvimi -h # help
nvimi -v # version
nvimi -c # print nvimpp.config
```

**Picker keys**

| Key | Action |
|---|---|
| `j` / `k`, Down / Up | move |
| Enter | open the selected entry |
| `q`, Esc, Ctrl-C | quit |

Entries marked `dashboard` are bare `nvimpp` launches. Selecting one opens Neovim in that folder. Files are opened through `nvimpp`, so a pick from the menu is recorded too.

<img src="media/divider.svg" width="100%" height="4">

## Captures
### nvimpp
<p align="center">
  <img src="media/nvimpp.gif" width="700" alt="nvimpp demo: opening files and jumping with Alt-Enter"><br>
  <code>The GIF shows some use cases of nvimpp, namely nvimpp, nvimpp filename, and the very useful find option using Alt-Enter.</code>
</p>

### nvimi
<p align="center">
  <img src="media/nvimi.gif" width="700" alt="nvimi demo: picker, stats and history"><br>
  <code>The GIF shows some use cases of nvimi, namely the default picker that lets you select your most used files ranked by frecency, the statistics of your files, and the history of your files without the need to open the history file.</code>
</p>

## Configuration

Two files in `~/.config/nvimpp`, both installed by `make`:

| File | Read by | Contents |
|---|---|---|
| `nvimpp.config` | `nvimpp` and `nvimi` | history size, picker rows, score half-life |
| `editors.config` | `nvimpp` | which editor to launch |

`nvimpp.config`:

```
# The inputs on the right of the equal sign have to be positive integers.
rows = 15
halflife_days = 7
limit = 10000
wait = 1000
```

| Key | Meaning |
|---|---|
| `limit` | number of history lines kept after a trim |
| `wait` | number of extra lines allowed past `limit` before the next trim |
| `rows` | maximum number of rows shown in the picker |
| `halflife_days` | half-life of the frecency score, in days |

Values must be positive whole numbers. A value that is 0, negative or not a number is ignored and the key keeps its default.

### Choosing the editor

`editors.config`:

```
editor = nvim
```

| Value | Editor launched |
|---|---|
| `nvim` (default) | Neovim |
| `vim` | Vim |

The value is case-insensitive, so `Vim` and `VIM` also select Vim. Any other value falls back to `nvim`. Both editors write to the same history, so the picker and `--find` see files opened with either one. The `--vim` flag forces Vim for a single launch.

### Format rules

These apply to both files:

* one `key = value` per line
* spaces and tabs are ignored, so `rows=15`, `rows = 15` and `  rows =15` are the same
* a line whose first non-blank character is `#` is a comment, so comments can be indented
* text after `#` on a setting line is a comment too: `editor = vim # my default`
* empty lines and lines with only spaces are skipped
* if a key appears twice, the last line wins

**Warnings.** If a file is missing, the program prints a warning and uses the defaults. A line without `=` (`rows 15`) or with an empty key or value (`rows =`, `= 15`) prints a warning naming the problem and is skipped. A key missing from the file keeps its default. `nvimi -c` (or `nvimi --config`) prints `nvimpp.config`.

<img src="media/divider.svg" width="100%" height="4">

## Scoring

Each launch of a file adds `0.5^(age / half-life)` to that file's score, where `age` is the time since the launch. With the default half-life of 7 days, a launch from today counts 1, one from a week ago counts 0.5, and one from a month ago about 0.05. Files you open often and recently rank first.

<img src="media/divider.svg" width="100%" height="4">

## History file

Location: `~/.local/share/nvimpp/history_nvim.csv`

One line per launch, three tab-separated fields:

```
Thu Nov 05 18:31:01 2026<TAB>/home/user/project/main.cpp<TAB>-R main.cpp
```

1. local date and time of the launch
2. absolute path of the file, or the current folder for a bare launch
3. the arguments as typed, or `NO_ARG` for a bare launch

The file is trimmed automatically. Once it grows past roughly `200 * (limit + wait)` bytes and holds more than `limit + wait` lines, the oldest lines are removed so that the newest `limit` remain. The trim writes a temporary file and renames it, so the history is never left half-written.

<img src="media/divider.svg" width="100%" height="4">

## Exit status

| Code | nvimpp | nvimi |
|---|---|---|
| 0 | | success, or the picker was closed without a selection |
| 1 | `HOME` not set, history file not writable, `--find` without a query, no match, unsupported shell for `--init` | invalid argument or option, `HOME` not set, history file missing, `-c` with no config file |
| 2 | | internal error: no mode selected |
| 4 | | no terminal available for the picker (`/dev/tty`) |
| 5 | | changing to the folder of a dashboard entry failed |
| 127 | the editor could not be started | `nvimpp` could not be started |

On success, `nvimpp` is replaced by the editor, so its exit status is the editor's.

<img src="media/divider.svg" width="100%" height="4">

## Man pages

After `make`, `man nvimpp` and `man nvimi` work from anywhere, as long as `~/.local/bin` is in your `PATH`. Without installing, use `man ./nvimpp.1` and `man ./nvimi.1` from the repository folder.

<img src="media/divider.svg" width="100%" height="4">

## Limitations and future work

**Current limitations**

* The `--find` match is a substring match, not a fuzzy one: `ac` matches `acme.cpp` but not `a.cpp`.
* Neovim flags that take a value (`-c`, `-u`, `--cmd` and others) are listed in the source. A value flag added in a future Neovim release would have its value recorded as a file name; Neovim itself still receives it correctly. The same list is used when the editor is Vim.
* Only `nvim` and `vim` can be selected as the editor for now.
* Paths and arguments are assumed to be ASCII for display purposes.
* Built and tested on Linux only (see [Compatibility](#compatibility)).

**Planned**

* Fuzzy matching for `--find`, so that `ac` also finds `a.cpp`.
* More editors in `editors.config` (Emacs, Helix, Kakoune, micro, nano).
* Inline search in the `nvimi` picker: type to filter the list instead of only scrolling.
* Cross-platform compatibility: tested support for macOS and the BSDs, and for Windows through WSL.

<img src="media/divider.svg" width="100%" height="4">

## Maintainer

<a href="https://github.com/Riccardo-Martelli">
  <img src="https://github.com/Riccardo-Martelli.png" width="90" alt="Riccardo Martelli">
</a>

**Riccardo Martelli**: [GitHub](https://github.com/Riccardo-Martelli) | [LinkedIn](https://www.linkedin.com/in/riccardomartelli/)

<img src="media/divider.svg" width="100%" height="4">

## License

This software is provided under the MIT license. [View License](LICENSE)
