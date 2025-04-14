import { defineConfig } from 'vitest/config';
import { SPEC_DIR } from './gulpfile.mjs';

export default defineConfig({
    test: {
        environment: 'jsdom',
        dir: SPEC_DIR,
    }
});
