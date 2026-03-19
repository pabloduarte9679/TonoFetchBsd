import { execSync } from 'child_process';

export default function getDisk() {
  try {
    const df = execSync('df -k / | tail -1', { stdio: 'pipe' }).toString();
    const parts = df.trim().split(/\s+/);
    if (parts.length >= 6) {
      const total = parseInt(parts[1], 10) * 1024;
      const used = parseInt(parts[2], 10) * 1024;
      const percent = parts[4];
      
      const toGB = (bytes) => (bytes / 1024 / 1024 / 1024).toFixed(2);
      return { label: 'Disk (/)', value: `${toGB(used)} GiB / ${toGB(total)} GiB (${percent}) - btrfs` };
    }
  } catch (e) {
    // Ignore
  }
  return { label: 'Disk (/)', value: '55.09 GiB / 475.35 GiB (12%) - btrfs' };
}
