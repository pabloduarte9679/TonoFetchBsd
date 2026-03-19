import os from 'os';

export default function getKernel() {
  const kernel = os.release();
  return { label: 'Kernel', value: `Linux ${kernel}` };
}
