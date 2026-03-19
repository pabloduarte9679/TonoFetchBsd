#!/usr/bin/env node

import { collectInfo, getHeader } from '../src/modules/index.js';
import { getLogo } from '../src/ascii/index.js';
import { getTheme } from '../src/themes/index.js';
import { loadConfig } from '../src/config.js';
import { renderOutput } from '../src/renderer.js';

function printHelp() {
  console.log(`
TonoFetch - Custom CLI System Info Tool

Usage: tonofetch [options]

Options:
  --minimal       Show only essential system info
  --json          Export info in JSON format
  --theme <name>  Set color theme (default, dracula, nord, gruvbox, catppuccin)
  --logo <name>   Set ASCII logo (ubuntu, tux, tonofetch) or path to a file
  --help          Show this help message

Configuration:
  You can create a config file at ~/.config/tonofetch/config.json
  Example content:
  {
    "theme": "dracula",
    "logo": "tux",
    "modules": ["os", "kernel", "uptime", "shell", "cpu", "ram"],
    "minimal": false
  }
`);
  process.exit(0);
}

function parseArgs() {
  const args = process.argv.slice(2);
  const cliOptions = {};

  for (let i = 0; i < args.length; i++) {
    const arg = args[i];
    if (arg === '--help') printHelp();
    if (arg === '--minimal') cliOptions.minimal = true;
    if (arg === '--json') cliOptions.json = true;
    if (arg === '--theme') {
      cliOptions.theme = args[i + 1];
      i++;
    }
    if (arg === '--logo') {
      cliOptions.logo = args[i + 1];
      i++;
    }
  }
  return cliOptions;
}

async function main() {
  const fileConfig = loadConfig();
  const cliOptions = parseArgs();

  // Merge options: CLI overrides File Config
  const options = { ...fileConfig, ...cliOptions };

  let enabledModules = options.modules;
  if (options.minimal) {
    enabledModules = ['os', 'kernel', 'cpu', 'ram'];
  }

  // Collect system info in parallel (async)
  const infoList = await collectInfo(enabledModules);

  if (options.json) {
    const jsonOutput = {
      header: getHeader(),
      info: infoList
    };
    console.log(JSON.stringify(jsonOutput, null, 2));
    return;
  }

  // Render text output
  const asciiLines = getLogo(options.logo);
  const theme = getTheme(options.theme);

  const output = renderOutput(asciiLines, infoList, theme);
  console.log(output);
}

main();
