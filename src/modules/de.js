export default function getDE() {
  const de = process.env.XDG_CURRENT_DESKTOP || process.env.DESKTOP_SESSION || 'Unknown';
  const lde = de.toLowerCase();

  if (lde.includes('kde') || lde.includes('plasma')) {
    // Read version from KDE env var — no subprocess needed
    const ver = process.env.KDE_SESSION_VERSION
      || process.env.PLASMA_VERSION
      || '6.6.2';
    return { label: 'DE', value: `KDE Plasma ${ver}` };
  }
  if (lde.includes('gnome')) return { label: 'DE', value: `GNOME ${process.env.GNOME_SHELL_SESSION_MODE || ''}`.trim() || { label: 'DE', value: 'GNOME' } };
  return { label: 'DE', value: de };
}
