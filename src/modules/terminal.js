export default function getTerminal() {
  // TERM_PROGRAM is set by most modern terminal emulators (konsole, kitty, iTerm2, etc.)
  const term = process.env.TERM_PROGRAM
    || process.env.COLORTERM
    || process.env.TERM
    || 'Unknown';

  // Konsole sets TERM_PROGRAM=konsole, also check KONSOLE_VERSION env var
  if (term.toLowerCase().includes('konsole')) {
    const ver = process.env.KONSOLE_VERSION;
    if (ver) {
      // KONSOLE_VERSION is an int like 250402; format as X.Y.Z
      const v = String(ver);
      const major = parseInt(v.slice(0, -4), 10);
      const minor = parseInt(v.slice(-4, -2), 10);
      const patch = parseInt(v.slice(-2), 10);
      return { label: 'Terminal', value: `konsole ${major}.${minor}.${patch}` };
    }
    return { label: 'Terminal', value: 'konsole' };
  }

  if (term.toLowerCase().includes('vscode')) return { label: 'Terminal', value: 'vscode' };
  if (term.toLowerCase().includes('kitty')) return { label: 'Terminal', value: `kitty ${process.env.KITTY_WINDOW_ID ? '' : ''}`.trim() || 'kitty' };
  if (term.toLowerCase().includes('alacritty')) return { label: 'Terminal', value: 'Alacritty' };

  return { label: 'Terminal', value: term };
}
