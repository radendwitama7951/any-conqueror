import { defineConfig } from 'vite';
import handlebars from 'vite-plugin-handlebars';
import { resolve } from 'path';

export default defineConfig({
  plugins: [
    handlebars({
      // Point this to where your partial HTML chunks live
      partialDirectory: resolve(__dirname, 'src/components'),
    }),
  ],
});
