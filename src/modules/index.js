import getOS from './os.js';
import getHost from './host.js';
import getKernel from './kernel.js';
import getUptime from './uptime.js';
import getPackages from './packages.js';
import getShell from './shell.js';
import getResolution from './resolution.js';
import getDE from './de.js';
import getWM from './wm.js';
import getTerminal from './terminal.js';
import getCPU from './cpu.js';
import getGPU from './gpu.js';
import getRAM from './ram.js';
import getDisk from './disk.js';
import getTheme from './theme.js';
import getWMTheme from './wmtheme.js';
import getIcons from './icons.js';
import getFont from './font.js';
import getCursor from './cursor.js';
import getSwap from './swap.js';
import getLocalIP from './localip.js';
import getBattery from './battery.js';
import getLocale from './locale.js';
import os from 'os';

export const ALL_MODULES = {
  os: getOS,
  host: getHost,
  kernel: getKernel,
  uptime: getUptime,
  packages: getPackages,
  shell: getShell,
  resolution: getResolution,
  de: getDE,
  wm: getWM,
  wmtheme: getWMTheme,
  theme: getTheme,
  icons: getIcons,
  font: getFont,
  cursor: getCursor,
  terminal: getTerminal,
  cpu: getCPU,
  gpu: getGPU,
  ram: getRAM,
  swap: getSwap,
  disk: getDisk,
  localip: getLocalIP,
  battery: getBattery,
  locale: getLocale
};

export function getHeader() {
  const username = os.userInfo().username;
  const hostname = os.hostname();
  return `${username}@${hostname}`;
}

// Wraps a module (sync or async) in a Promise and swallows errors
function runModule(fn) {
  try {
    return Promise.resolve(fn());
  } catch (e) {
    return Promise.resolve(null);
  }
}

// Runs all enabled modules IN PARALLEL for maximum speed
export async function collectInfo(enabledModules = Object.keys(ALL_MODULES)) {
  const entries = enabledModules
    .filter(id => ALL_MODULES[id])
    .map(id => ({ id, fn: ALL_MODULES[id] }));

  const results = await Promise.all(
    entries.map(({ fn }) => runModule(fn))
  );

  const output = [];
  for (let i = 0; i < results.length; i++) {
    const data = results[i];
    if (data && data.value !== 'Unknown') {
      output.push({ id: entries[i].id, ...data });
    }
  }
  return output;
}
