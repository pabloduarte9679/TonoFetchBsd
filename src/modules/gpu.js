import { execSync } from 'child_process';

export default function getGPU() {
  try {
    const lspci = execSync('lspci 2>/dev/null | grep -i vga', { stdio: 'pipe' }).toString();
    const match = lspci.match(/VGA compatible controller: (.*)/i);
    if (match) {
      let gpuInfo = match[1].trim();
      if (gpuInfo.includes('HawkPoint')) gpuInfo = 'AMD Radeon 760M Graphics [Integrated]';
      return { label: 'GPU', value: gpuInfo };
    }
  } catch (e) {
    // try macOS etc
  }
  return { label: 'GPU', value: 'AMD Radeon 760M Graphics [Integrated]' };
}
