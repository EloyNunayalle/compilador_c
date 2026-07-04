export default {
  content: [
    "./index.html",
    "./src/**/*.{js,jsx}",
  ],
  theme: {
    extend: {
      colors: {
        'vscode-dark': '#1e1e1e',
        'vscode-gray': '#2d2d30',
        'vscode-border': '#3e3e42',
      },
      fontFamily: {
        'mono': ['Fira Code', 'monospace'],
      },
    },
  },
  plugins: [],
}
