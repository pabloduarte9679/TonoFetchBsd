import fs from 'fs';
import { logos } from './logos.js';

export function getLogo(nameOrPath) {
  if (!nameOrPath) return logos.default;
  
  const nameLower = nameOrPath.toLowerCase();
  if (logos[nameLower]) {
    return logos[nameLower];
  }
  
  // Try to load from file
  try {
    if (fs.existsSync(nameOrPath)) {
      const content = fs.readFileSync(nameOrPath, 'utf8');
      return content.split('\n');
    }
  } catch(e) {
    // Ignore and fallback
  }

  return logos.default;
}
