# nvim2 and nvimi

Frecency tracking for Neovim, in the spirit of zoxide and atuin.

* **nvim2** is a thin wrapper around `nvim`. It records every launch (date, resolved file path, arguments) in a history file, then hands over to Neovim with the arguments untouched. With `--find` it jumps to the best matching file from the history instead of opening the literal name.
* **nvimi** reads that history. Its default mode is an inline picker that ranks files by frecency (frequency weighted by recency) and opens the selected one. It also has a `stats` and a `history` mode.

## Requirements

* Linux, a C++17 compiler (`g++`), `make`
* Neovim (`nvim`) in `PATH`
* bash or zsh, for the optional Alt-Enter binding

## Build and install

```bash
make
make install
```

This installs `nvim2` and `nvimi` into `~/.local/bin`. Make sure that folder is in your `PATH`. To install somewhere else:

```bash
make install PREFIX=/some/prefix
```

## Setup

Add to `~/.bashrc` (or `~/.zshrc`):

```bash
alias nvim=nvim2
eval "$(nvim2 --init bash)" # or: eval "$(nvim2 --init zsh)"
```

The alias makes every `nvim` you type go through `nvim2`. Programs that call Neovim themselves (`git commit`, `sudoedit`, scripts) still use the real `nvim`, so their files are not recorded. Typing `\nvim` or `command nvim` also skips the wrapper.

The `eval` line installs the Alt-Enter key binding described below.

Create the config file `~/.config/nvim2/nvim2.config` (see [Configuration](#configuration)). `nvimi` exits with an error if it is missing.

## Usage

### nvim2

```bash
nvim2 file.cpp # open file.cpp and record it
nvim2 a.cpp b.cpp # each file gets its own history line
nvim2 # no file: records the current folder as a dashboard entry
nvim2 -R notes.md # Neovim flags are passed through unchanged
nvim2 query --find # open the best match for "query" from the history
```

**Enter vs Alt-Enter.** With the binding installed, a line starting with `nvim ` or `nvim2 ` behaves like this:

* **Enter** opens exactly what you typed. `nvim main` opens or creates the file `main`.
* **Alt-Enter** appends `--find`. `nvim main` then opens the most used and most recent file in the history whose name contains `main`, for example `~/project/src/main.cpp`.

**How `--find` matches. (use alt+Enter is cooler)** All non-flag words form one query.

* Matching is case-insensitive.
* The words must appear in the file path in the order typed.
* The last word must appear in the file name itself, not only in a folder name.
* Only files that still exist on disk are candidates.
* The candidate with the highest frecency score wins. If nothing matches, `nvim2` prints `no match for the query` and Neovim does not start.

```bash
nvim2 conway cgof --find # matches .../conwaygof/cgof.cpp
nvim2 cgof conway --find # no match: wrong order
```

### nvimi

```bash
nvimi # inline picker, ranked by frecency
nvimi stats # files ranked by number of launches
nvimi history # every launch with date and time since
nvimi -h # help
nvimi -v # version
```

**Picker keys**

| Key | Action |
|---|---|
| `j` / `k`, Down / Up | move |
| Enter | open the selected entry |
| `q`, Esc, Ctrl-C | quit |

Entries marked `dashboard` are bare `nvim2` launches. Selecting one opens Neovim in that folder. Files are opened through `nvim2`, so a pick from the menu is recorded too.

## Configuration

File: `~/.config/nvim2/nvim2.config`

```
# nvim2 / nvimi configuration
limit = 10000
wait = 1000
rows = 15
halflife_days = 7
```

| Key | Meaning |
|---|---|
| `limit` | number of history lines kept after a trim |
| `wait` | number of extra lines allowed past `limit` before the next trim |
| `rows` | maximum number of rows shown in the picker |
| `halflife_days` | half-life of the frecency score, in days |

Format rules:

* one `key = value` per line, with a space on each side of `=`
* values are whole numbers
* lines starting with `#` are comments, and so are empty lines

`nvim2` falls back to the defaults above if the file is missing. Set all four keys.

## Scoring

Each launch of a file adds `0.5^(age / half-life)` to that file's score, where `age` is the time since the launch. With the default half-life of 7 days, a launch from today counts 1, one from a week ago counts 0.5, and one from a month ago about 0.05. Files you open often and recently rank first.

## History file

Location: `~/.local/share/nvim2/history_nvim.csv`

One line per launch, three tab-separated fields:

```
Thu Nov 05 18:31:01 2026<TAB>/home/user/project/main.cpp<TAB>-R main.cpp
```

1. local date and time of the launch
2. absolute path of the file, or the current folder for a bare launch
3. the arguments as typed, or `NO_ARG` for a bare launch

The file is trimmed automatically. Once it grows past roughly `200 * (limit + wait)` bytes and holds more than `limit + wait` lines, the oldest lines are removed so that the newest `limit` remain. The trim writes a temporary file and renames it, so the history is never left half-written.

## Exit status

| Code | nvim2 | nvimi |
|---|---|---|
| 0 | | success, or the picker was closed without a selection |
| 1 | `HOME` not set, history file not writable, `--find` without a query, no match, unsupported shell for `--init` | invalid argument or option, `HOME` not set, history file missing |
| 2 | | internal error: no mode selected |
| 3 | | config file missing, or changing to a dashboard folder failed |
| 4 | | no terminal available for the picker (`/dev/tty`) |
| 127 | Neovim could not be started | `nvim2` could not be started |

On success, `nvim2` is replaced by Neovim, so its exit status is Neovim's.

## Man pages

`man ./nvim2.1` and `man ./nvimi.1` from the repository folder.

## Limitations

* The `--find` match is a substring match, not a fuzzy one: `ac` matches `acme.cpp` but not `a.cpp`.
* Neovim flags that take a value (`-c`, `-u`, `--cmd` and others) are listed in the source. A value flag added in a future Neovim release would have its value recorded as a file name; Neovim itself still receives it correctly.
* Paths and arguments are assumed to be ASCII for display purposes.
