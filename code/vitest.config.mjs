import * as path from 'node:path';

import { MODULE_DEV } from './html/lib/preset.mjs';
import {
    PRESET_DIR,
    SPEC_DIR,
    VENDOR_DIR,
} from './gulpfile.mjs';

import { defineConfig } from 'vitest/config';

export default defineConfig({
    test: {
        environment: 'jsdom',
        dir: SPEC_DIR,
    },
    // vite-specific overrides, both for the build and importmap helper script
    resolve: {
        alias: [
            {find: '/vendor', replacement: VENDOR_DIR},
            {find: '@build-preset', replacement: path.join(PRESET_DIR, MODULE_DEV)},
        ],
    },
});
