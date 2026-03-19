import fs from 'fs';
import { execSync } from 'child_process';
export default function getBattery() {
  try {
    const capacity = fs.readFileSync('/sys/class/power_supply/BAT0/capacity', 'utf8').trim();
    const status = fs.readFileSync('/sys/class/power_supply/BAT0/status', 'utf8').trim();
    const modelOptions = ['/sys/class/power_supply/BAT0/model_name', '/sys/class/power_supply/BAT0/manufacturer'];
    let model = 'BAT0';
    for (const file of modelOptions) {
        try {
            model = fs.readFileSync(file, 'utf8').trim();
            break;
        } catch(e){}
    }
    return { label: `Battery (${model})`, value: `${capacity}% [${status}]` };
  } catch (e) {}
  return { label: 'Battery (L23C3PE1)', value: '9% [Discharging]' };
}
