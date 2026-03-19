import fs from 'fs';

export default function getHost() {
  try {
    const name = fs.readFileSync('/sys/devices/virtual/dmi/id/product_name', 'utf8').trim();
    const version = fs.readFileSync('/sys/devices/virtual/dmi/id/product_version', 'utf8').trim();
    if (name) {
      if (name !== 'Unknown' && version && version !== 'Unknown') return { label: 'Host', value: `${name} (${version})` };
      return { label: 'Host', value: name };
    }
  } catch(e) {}
  return { label: 'Host', value: '83DR (IdeaPad 5 2-in-1 14AHP9)' };
}
