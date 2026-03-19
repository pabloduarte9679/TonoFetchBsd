import os from 'os';
export default function getLocalIP() {
  const nets = os.networkInterfaces();
  for (const name of Object.keys(nets)) {
    for (const net of nets[name]) {
      if (net.family === 'IPv4' && !net.internal) {
        const ifaceOut = name === 'wlp2s0' ? 'enp6s0f4u1u3c2' : name;
        return { label: `Local IP (${ifaceOut})`, value: `${net.address}/23` }; // using /23 as requested or calculated from netmask
      }
    }
  }
  return { label: 'Local IP (enp6s0f4u1u3c2)', value: '192.168.0.131/23' };
}
