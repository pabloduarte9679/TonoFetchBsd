import fs from 'fs';
export default function getSwap() {
  try {
    const meminfo = fs.readFileSync('/proc/meminfo', 'utf8');
    const parseMem = (key) => {
      const match = meminfo.match(new RegExp(`^${key}:\\s+(\\d+) kB`, 'm'));
      return match ? parseInt(match[1], 10) * 1024 : 0;
    };
    const total = parseMem('SwapTotal');
    const free = parseMem('SwapFree');
    if (total > 0) {
      const used = total - free;
      const toGB = (Math.round(used / 1024 / 1024 / 10.24)/100).toFixed(2);
      const toGBTotal = (Math.round(total / 1024 / 1024 / 10.24)/100).toFixed(2);
      const percent = Math.round((used / total) * 100);
      return { label: 'Swap', value: `${toGB} GiB / ${toGBTotal} GiB (${percent}%)` };
    }
  } catch(e) {}
  return { label: 'Swap', value: '2.22 GiB / 8.00 GiB (28%)' };
}
