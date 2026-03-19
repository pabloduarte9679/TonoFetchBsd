import fs from 'fs';
import os from 'os';

export default function getRAM() {
  try {
    const meminfo = fs.readFileSync('/proc/meminfo', 'utf8');
    const parseMem = (key) => {
      const match = meminfo.match(new RegExp(`^${key}:\\s+(\\d+) kB`, 'm'));
      return match ? parseInt(match[1], 10) * 1024 : 0;
    };
    
    const total = parseMem('MemTotal');
    const available = parseMem('MemAvailable');
    
    if (total > 0 && available > 0) {
      const used = total - available;
      const toGB = (bytes) => (bytes / 1024 / 1024 / 1024).toFixed(2);
      const percent = Math.round((used / total) * 100);
      return { label: 'Memory', value: `${toGB(used)} GiB / ${toGB(total)} GiB (${percent}%)` };
    }
  } catch(e) {}

  // Fallback
  const total = os.totalmem();
  const free = os.freemem();
  const used = total - free;
  const toGB = (bytes) => (bytes / 1024 / 1024 / 1024).toFixed(2);
  const percent = Math.round((used / total) * 100);
  return { label: 'Memory', value: `${toGB(used)} GiB / ${toGB(total)} GiB (${percent}%)` };
}
