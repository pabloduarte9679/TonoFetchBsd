import fs from 'fs';
import os from 'os';
import path from 'path';

export function loadConfig() {
  const defaults = {
    theme: 'default',
    logo: 'tonofetch',
    modules: ['os', 'host', 'kernel', 'uptime', 'packages', 'shell', 'resolution', 'de', 'wm', 'wmtheme', 'theme', 'icons', 'font', 'cursor', 'terminal', 'cpu', 'gpu', 'ram', 'swap', 'disk', 'localip', 'battery', 'locale'],
    minimal: false,
    json: false
  };

  const configPath = path.join(os.homedir(), '.config', 'tonofetch', 'config.json');
  try {
    if (fs.existsSync(configPath)) {
      const userConfig = JSON.parse(fs.readFileSync(configPath, 'utf8'));
      return { ...defaults, ...userConfig };
    }
  } catch(e) {
    // Ignore invalid config
  }
  return defaults;
}
