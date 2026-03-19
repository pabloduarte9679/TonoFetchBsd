import { getHeader } from './modules/index.js';
import chalk from 'chalk';

export function renderOutput(asciiLines, infoList, theme) {
  const header = getHeader();
  const sepLength = header.length;
  const separator = theme.separator('-'.repeat(sepLength));

  // Combine header + info into one array of lines
  const rightLines = [
    theme.title(header),
    separator
  ];

  for (const info of infoList) {
    const label = theme.label(info.label);
    const value = info.value;
    rightLines.push(`${label}: ${value}`);
  }

  // Draw simple terminal color blocks at the bottom
  const c1 = [chalk.bgBlack, chalk.bgRed, chalk.bgGreen, chalk.bgYellow, chalk.bgBlue, chalk.bgMagenta, chalk.bgCyan, chalk.bgWhite];
  const c2 = [chalk.bgBlackBright, chalk.bgRedBright, chalk.bgGreenBright, chalk.bgYellowBright, chalk.bgBlueBright, chalk.bgMagentaBright, chalk.bgCyanBright, chalk.bgWhiteBright];
  
  const drawBlocks = (arr) => arr.map(c => c('   ')).join('');
  rightLines.push(''); // spacing
  rightLines.push(drawBlocks(c1));
  rightLines.push(drawBlocks(c2));

  // Find max width of ASCII art
  let maxAsciiWidth = 0;
  for (const line of asciiLines) {
    if (line.length > maxAsciiWidth) {
      maxAsciiWidth = line.length;
    }
  }

  const outputLinesCount = Math.max(asciiLines.length, rightLines.length);
  const finalOutput = [];

  for (let i = 0; i < outputLinesCount; i++) {
    const rawAsciiLine = asciiLines[i] || '';
    const paddedAscii = rawAsciiLine.padEnd(maxAsciiWidth, ' ');
    const coloredAscii = theme.ascii(paddedAscii);
    
    const rightLine = rightLines[i] || '';
    
    finalOutput.push(`${coloredAscii}  ${rightLine}`);
  }

  return finalOutput.join('\n');
}
