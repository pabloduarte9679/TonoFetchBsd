export default function getWM() {
  const de = process.env.XDG_CURRENT_DESKTOP || process.env.DESKTOP_SESSION || '';
  const lowerDe = de.toLowerCase();
  
  if (lowerDe.includes('plasma') || lowerDe.includes('kde')) {
    const session = process.env.XDG_SESSION_TYPE === 'wayland' ? ' (Wayland)' : ' (X11)';
    return { label: 'WM', value: `KWin${session}` };
  }
  if (lowerDe.includes('gnome')) return { label: 'WM', value: 'Mutter' };
  if (lowerDe.includes('xfce')) return { label: 'WM', value: 'Xfwm4' };
  if (lowerDe.includes('mate')) return { label: 'WM', value: 'Marco' };
  if (lowerDe.includes('cinnamon')) return { label: 'WM', value: 'Muffin' };
  
  // Try XDG_SESSION_TYPE for Wayland generic
  if (process.env.XDG_SESSION_TYPE === 'wayland') return { label: 'WM', value: 'Wayland Compositor' };
  
  return { label: 'WM', value: process.env.XDG_SESSION_DESKTOP || 'Unknown' };
}
