import { execFile } from 'child_process';
import { promisify } from 'util';

const execFileAsync = promisify(execFile);

async function tryCount(cmd, args, adjustFn = (n) => n) {
  try {
    const { stdout } = await execFileAsync(cmd, args, { timeout: 3000 });
    const n = parseInt(stdout.trim(), 10);
    if (!isNaN(n) && n > 0) return adjustFn(n);
  } catch (e) {}
  return null;
}

export default async function getPackages() {
  const [dpkg, rpm, flatpak, snap] = await Promise.all([
    tryCount('sh', ['-c', 'dpkg-query -f \'.\n\' -W 2>/dev/null | wc -l']),
    tryCount('sh', ['-c', 'rpm -qa 2>/dev/null | wc -l']),
    tryCount('sh', ['-c', 'flatpak list 2>/dev/null | wc -l']),
    // snap list includes a header line, subtract 1
    tryCount('sh', ['-c', 'snap list 2>/dev/null | wc -l'], (n) => n > 1 ? n - 1 : null),
  ]);

  const counts = [];
  if (dpkg)    counts.push(`${dpkg} (dpkg)`);
  if (rpm)     counts.push(`${rpm} (rpm)`);
  if (flatpak) counts.push(`${flatpak} (flatpak)`);
  if (snap)    counts.push(`${snap} (snap)`);

  if (counts.length === 0) return { label: 'Packages', value: 'Unknown' };
  return { label: 'Packages', value: counts.join(', ') };
}
