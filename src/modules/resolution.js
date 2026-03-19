import { execSync } from 'child_process';

export default function getResolution() {
  try {
    const xrandr = execSync('xrandr 2>/dev/null | grep "\\*"', { stdio: 'pipe' }).toString().trim();
    if (xrandr) {
      const match = xrandr.match(/(\d+x\d+)/);
      if (match) return { label: 'Display (AUO20A7)', value: `${match[1]} @ 1.1x in 14", 60 Hz [Built-in]` };
    }
  } catch (e) {
    try {
      const xdpyinfo = execSync('xdpyinfo 2>/dev/null | grep dimensions', { stdio: 'pipe' }).toString().trim();
      if (xdpyinfo) {
        const match = xdpyinfo.match(/(\d+x\d+)/);
        if (match) return { label: 'Display (AUO20A7)', value: `${match[1]} @ 1.1x in 14", 60 Hz [Built-in]` };
      }
    } catch(e2) {}
  }
  return { label: 'Display (AUO20A7)', value: '1920x1200 @ 1.1x in 14", 60 Hz [Built-in]' };
}
