import os from 'os';

export default function getUptime() {
  const uptime = os.uptime();
  const days = Math.floor(uptime / 86400);
  const hours = Math.floor((uptime % 86400) / 3600);
  const minutes = Math.floor((uptime % 3600) / 60);

  let parts = [];
  if (days > 0) parts.push(`${days} days`);
  if (hours > 0) parts.push(`${hours} hours`);
  if (minutes > 0) parts.push(`${minutes} mins`);

  if (parts.length === 0) return { label: 'Uptime', value: '< 1 min' };

  return { label: 'Uptime', value: parts.join(', ') };
}
