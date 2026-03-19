# TonoFetch

TonoFetch is a custom, fast, and highly customizable CLI system information tool inspired by `fastfetch` and `neofetch`. It displays system information visually in the terminal alongside an ASCII logo, with support for themes, modules, and JSON export.

This is an attempt to port this freebsd
## Features

- **System Information**: Displays OS, Kernel, Uptime, Shell, CPU, RAM, Disk, etc.
- **Customizable ASCII Logos**: Choose from built-in logos or create your own.
- **Theming**: Several built-in color themes to match your terminal setup (Default, Dracula, Nord, Gruvbox, Catppuccin).
- **Extensible configuration**: Configure modules, logos, and themes using a JSON configuration file.
- **Export options**: View the output in the standard visual mode, a minimal mode, or export it to JSON.

## Installation

### Prerequisites

- GCC (or any C compiler)
- `make`

### Build

1. Clone or download this repository.
2. Navigate to the project directory:

```bash
cd TonoFetch
```

3. Build the binary:

```bash
make
```

This produces a `tonofetch` binary in the project root.

4. (Optional) Install system-wide:

```bash
sudo cp tonofetch /usr/local/bin/
```

## Usage

Simply run the command in your terminal:

```bash
./tonofetch
```

### Options

TonoFetch supports several command-line arguments to modify its behavior on the fly:

- `--minimal`       : Show only essential system info (OS, Kernel, CPU, and RAM).
- `--json`          : Export the gathered information in JSON format (useful for scripting or integration with other tools).
- `--theme <name>`  : Set the color theme. Built-in themes: `default`, `dracula`, `nord`, `gruvbox`, `catppuccin`.
- `--logo <name>`   : Set the ASCII logo. Built-in logos: `ubuntu`, `tux`, `tonofetch`. You can also provide a path to a custom file.
- `--help`          : Show the help message and exit.

**Examples:**

```bash
# Run with the Dracula theme and Tux logo
./tonofetch --theme dracula --logo tux

# Run in minimal mode
./tonofetch --minimal

# Export system info as JSON
./tonofetch --json
```

## Configuration

You can configure TonoFetch permanently by creating a configuration file at `~/.config/tonofetch/config.json`. The CLI arguments will override the settings in this file.

### Example Configuration

Create the file `~/.config/tonofetch/config.json` with the following content:

```json
{
  "theme": "dracula",
  "logo": "tux",
  "modules": ["os", "kernel", "uptime", "shell", "cpu", "ram"],
  "minimal": false
}
```

- **`theme`**: Default theme string.
- **`logo`**: Default ASCII art string.
- **`modules`**: Array of string module names you want to display.
- **`minimal`**: Boolean to enable or disable minimal output by default.

## License

ISC License
