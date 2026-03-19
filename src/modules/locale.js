export default function getLocale() {
  const lang = process.env.LANG || 'en_US.UTF-8';
  return { label: 'Locale', value: lang };
}
