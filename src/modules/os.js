import fs from 'fs';
import os from 'os';

export default function getOS() {
  try {
    const osRelease = fs.readFileSync('/etc/os-release', 'utf8');
    const match = osRelease.match(/^PRETTY_NAME="(.*)"/m) || osRelease.match(/^PRETTY_NAME=(.*)/m);
    if (match) {
      const arch = os.arch() === 'x64' ? 'x86_64' : os.arch();
      return { label: 'OS', value: `${match[1]} ${arch}` };
    }
  } catch (e) {
    // Ignore
  }
  return { label: 'OS', value: 'Linux (Unknown)' };
}
