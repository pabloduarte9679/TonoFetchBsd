import chalk from 'chalk';

export const themes = {
  default: {
    title: chalk.bold.cyan,
    separator: chalk.gray,
    label: chalk.bold.blue,
    ascii: chalk.cyan
  },
  dracula: {
    title: chalk.bold.hex('#bd93f9'),
    separator: chalk.hex('#6272a4'),
    label: chalk.bold.hex('#8be9fd'),
    ascii: chalk.hex('#bd93f9')
  },
  nord: {
    title: chalk.bold.hex('#88c0d0'),
    separator: chalk.hex('#4c566a'),
    label: chalk.bold.hex('#81a1c1'),
    ascii: chalk.hex('#88c0d0')
  },
  gruvbox: {
    title: chalk.bold.hex('#fe8019'),
    separator: chalk.hex('#a89984'),
    label: chalk.bold.hex('#fabd2f'),
    ascii: chalk.hex('#fe8019')
  },
  catppuccin: {
    title: chalk.bold.hex('#cba6f7'),
    separator: chalk.hex('#6c7086'),
    label: chalk.bold.hex('#89b4fa'),
    ascii: chalk.hex('#cba6f7')
  }
};

export function getTheme(themeName) {
  if (!themeName) return themes.default;
  const lower = themeName.toLowerCase();
  return themes[lower] || themes.default;
}
