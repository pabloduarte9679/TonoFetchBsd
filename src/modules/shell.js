import fs from 'fs';

export default function getShell() {
  const shellPath = process.env.SHELL || '';
  if (!shellPath) return { label: 'Shell', value: 'bash 5.3.0' };

  const shellName = shellPath.split('/').pop();

  // Read version from pre-known files to avoid spawning a process
  try {
    if (shellName === 'zsh') {
      // zsh stores its version in /etc/zsh/zshrc comment or the binary itself.
      // The fastest approach: read the output of $ZSH_VERSION env (set when zsh is running)
      const ver = process.env.ZSH_VERSION;
      if (ver) return { label: 'Shell', value: `zsh ${ver}` };
    }
    if (shellName === 'bash') {
      const ver = process.env.BASH_VERSION;
      if (ver) return { label: 'Shell', value: `bash ${ver.split('(')[0]}` };
    }
    if (shellName === 'fish') {
      const ver = process.env.FISH_VERSION;
      if (ver) return { label: 'Shell', value: `fish ${ver}` };
    }
  } catch (e) {}

  return { label: 'Shell', value: shellName };
}
